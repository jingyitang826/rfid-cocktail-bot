const express = require("express");
const axios = require("axios");

const app = express();
const port = 9090;

let callbacks = { order: null, find_position: null};
let ingredients = [];
let order_list = [];
let order_history = [];
let order_number = 0;
let clearOrderTimeout = null;
let tokenTimestamps = new Map(); // Store token timestamps individually
const ingredientMap = {  // map RFID tokenID to the ingredients
        "0": "Brandy",
        "1": "Gin",
        "2": "Rum",
        "3": "Vodka",
        "4": "Whiskey",
        "5": "Sweet Vermouth",
        "6": "Soda",
        "7": "Lemon Juice",
        "8": "Syrup",
        "9": "Lime"
    };
let weightcell = [0,0,0,0];
let serve_order = null;
let drink_placement = [0,0,0,0];

app.use(express.json());
app.use(express.urlencoded({ extended: true }));

app.post("/server_refresh", (req, res) => {
    // Reset all variables
    ingredients = [];
    order_list = [];
    order_history = [];
    order_number = 0;
    serve_order = null;
    drink_placement = [0,0,0,0];
    tokenTimestamps = new Map();

    res.status(200).json({ message: "Server data has been refreshed." });
});
// Send UPDATE to the CPEE via Callback-Url
function notifyCpee(type, data) {
    const url = callbacks[type];
    if (!url) return console.warn(`No CPEE ${type} Callback Url registered`);
    axios.put(url, data, { headers: { "Content-Type": "application/json", "CPEE-UPDATE": "true" } })
        .catch(error => console.error(`Error sending ${type} event to CPEE:`, error.message));
}

// Store the Callback-Url
app.post("/:type/subscribe", (req, res) => {
    const type = req.params.type;
    if (!(type in callbacks)) return res.status(400).json({ status: 0, error: "Invalid subscription type" });
    callbacks[type] = req.header("CPEE-CALLBACK") || null;
    if (!callbacks[type]) return res.status(400).json({ status: 0, error: "Missing CPEE-CALLBACK header" });
    res.setHeader("Content-Type", "application/json");
    res.setHeader("CPEE-UPDATE", "true");
    res.json({ message: `CPEE ${type} Callback Url registered`, callbackUrl: callbacks[type] });
    console.log(`CPEE ${type} Callback Url registered:`, callbacks[type]);
});

// ===== RFID Reader =====
app.post("/rfid_reader/tokens", (req, res) => {
    const { tag_data } = req.body; 
    console.log("Received request from RFID:", req.body);
    
    const token = extractEPC(tag_data);
    if (!token) {
        return res.status(400).json({ status: 0 });
    }

    // time-based filter to prevent redundant reads
    const currentTime = Date.now();
    const lastReceivedTime = tokenTimestamps.get(token) || 0;
    if (currentTime - lastReceivedTime < 1000) {
        console.log("Duplicate token detected within 1 second, ignoring:", token);
        return res.status(200).json({ status: 1, message: "Duplicate ignored" });
    }
    tokenTimestamps.set(token, currentTime);

    // if Ingredients < 4
    if(ingredients.length < 4){
        const ingredient = ingredientMap[token];
        // update ingredients
        ingredients.push(ingredient);
    }

    res.status(200).json({ status: 1 });
});

// Function to extract the EPC value
function extractEPC(tag_data) {
    const match = tag_data.match(/\d\b/g);
    return match ? match[match.length - 1] : null;
}

// ===== Cancel Button =====
app.post("/cancel", (req, res) => {
    console.log("User Pressed the Cancel Button");
    ingredients = [];
    res.status(200).json({ status: 1 });
});

// ===== Confirm =====
app.post("/confirm", (req, res) => {
    console.log("User Pressed the Confirm Button");
    res.status(200).json({ status: 1 });
    if(ingredients.length>0){
        
        // loop until a unique order_number is found
        do {
            order_number = getRandomOrderNumber();
        } while (order_list.some(order => order.order_number === order_number));

        let order_detail = { order_number, ingredients };

        // send the Order to the CPEE
        notifyCpee("order", { order_detail });

        // clear the Selection
        ingredients = [];
        
        // clear the last timeout (if exists)         
        if (clearOrderTimeout) {
            clearTimeout(clearOrderTimeout);
        }

        // new timeout: display order number for 10 sec
        clearOrderTimeout = setTimeout(() => {
            console.log("Order Number Cleared");
            order_number = 0;
        }, 10000);
    };
});

// Generate a random number between 100 and 999
function getRandomOrderNumber() {
    return Math.floor(Math.random() * (999 - 100 + 1)) + 100;
}

// ===== Weight Cell for Pickup Station ===== 
// Weightcell Sends Req to the Server When a Change Occurs
app.post("/weightcell", (req, res) => {
    console.log("Received data from Weightcell:", req.body);
    const { LoadCell, status } = req.body;

    if (LoadCell >= 0 && LoadCell <= 3) {  
        // update the weightcell data  
        if (LoadCell === 3) {
            // weightcell 3 is reversed
            load_status = status === 'Empty' ? 1 : 0; 
        }
        else {
            load_status = status === 'Occupied' ? 1 : 0; 
        }
        res.status(200).json({ message: 'Weightcell updated', status: 1 });
        weightcell[LoadCell] = load_status;
        console.log("Weightcell updated:",LoadCell, "status:", weightcell);
        
        // detect drink pickup
        if (drink_placement[LoadCell] !== 0 && weightcell[LoadCell] === 0) {
            drink_placement[LoadCell] = 0;
        }
    } else {
        res.status(400).json({ error: 'Invalid LoadCell index' });
    }
});

app.post("/find_position", (req, res) => {
    serve_order = JSON.parse(req.body.serve_order);
    // find index of first occurrence of 0
    let available_position = weightcell.indexOf(0);

    if (available_position !== -1) {
        // update drink placement
        drink_placement[available_position] = serve_order.order_number;

        // find where to grab the glass
        let remainder = order_history.length % 10;
        let [glass_x, glass_y] = remainder < 5 ? [0, remainder] : [1, remainder - 5];

        res.status(200).json({ drink_placement, glass_x, glass_y, available_position });
    } else {
        // store the CALLBACK URL
        callbacks["find_position"] = req.header("CPEE-CALLBACK") || null;
        res.setHeader("CPEE-CALLBACK", "true");
        res.status(200).json({ message: "Waiting for available position..." });

        // retry every 1 sec
        let retryInterval = setInterval(() => {
            let available_position = weightcell.indexOf(0);
            if (available_position !== -1) {
                clearInterval(retryInterval);

                drink_placement[available_position] = serve_order.order_number;

                let remainder = order_history.length % 10;
                let [glass_x, glass_y] = remainder < 5 ? [0, remainder] : [1, remainder - 5];

                // send back to CPEE
                axios.put(callbacks["find_position"], { drink_placement, glass_x, glass_y, available_position }, { headers: { "Content-Type": "application/json"} })
                    .catch(error => console.error(`Error sending ${type} event to CPEE:`, error.message));

                // clear the CALLBACK URL
                callbacks["find_position"] = null;
            }
        }, 1000);
    }
});

app.post("/order_finish", (req, res) => {
    console.log('Order served:', serve_order);
    order_history.push(serve_order);
    order_list.shift();
    serve_order = null;
    res.status(200).json({ status:1 });
});


// ===== LED Module =====
// Respond with the Stored Data
app.get("/led/input", async (req, res) => {
    let response = {
        ingredients: ingredients.length === 0 ? "Not ingredients yet" : ingredients.filter(item => item !== null).join(","),
        order_number: order_number === 0 ? "Order not confirmed" : `Order number: ${order_number}`
    };
    res.status(200).json(response);
});

app.get("/led/output", async (req, res) => {
    // console.log('respond to LED:', drink_placement);
    let response = {
        drink_placement: drink_placement.map((value, index) => `${index}:${value === 0 ? " " : value}`).join(", ")
    };
    res.status(200).json(response);
});

app.listen(port, () => {
    console.log(`Server running on https://lehre.bpm.in.tum.de/ports/${port}`);
});