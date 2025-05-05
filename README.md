## Overview

`rfid-cocktail-bot` is a smart, interactive cocktail ordering system that integrates RFID token-based ingredient selection, LED-based information displays, a JavaScript server, and a UR cobot for automated drink delivery. The system logic is orchestrated via the CPEE process engine.

## System Architecture

- **RFID Tokens**: Represent individual drink ingredients.
- **LED Display Module (Input)**: Shows selected ingredients, confirmed order number, and the status of drink placements.
- **UR Cobot**: Executes drink delivery actions.
- **JavaScript Server**: Implements and handles CPEE process endpoints, acting as a communication bridge between the CPEE engine and the hardware interface layer.
- **CPEE Process Engine**: Manages the process workflow and communicates with the server via HTTP requests.


## Process Workflow

The drink-serving logic is orchestrated by the CPEE process model:

<img width="300" alt="image" src="https://github.com/user-attachments/assets/33163a6b-ed49-441a-8a57-dc175ca2b6cd" />


### Main Steps:

1. **Refresh the Server**: Clears all variables stored from the previous run.  
#### branch 1
1. **Wait for Orders**:  
   Sends a `CALLBACK` URL to the server and waits.  
   When a new order is submitted by a user, the server responds with an `UPDATE`, pushing the order into the `order_list` array in CPEE.
#### branch 2
1. **Check Order List**:  
   Continuously loops to check if there is at least one pending order.
2. **Fulfill First Order**:  
   Retrieves the first order from the `order_list` and assigns it to the `serve_order` variable.
3. **Timeout (Preparation Simulation)**:  
   Simulates a drink preparation delay.
4. **Find an Available Position**  
   Sends a request to the server to get `glass_x, glass_y, available_position`.  
   - A fixed 2×5 grid of glasses is used cyclically, so refilling is only needed after every 10 orders.
   - The server determines the available output position based on `weight cell` readings. If all positions are occupied, it stores the `CALLBACK` URL and retries every second until one becomes free.
   - The LED display is updated with the assigned output position.
5. **Send Coordinates to UR Cobot**:  
   The CPEE sends `glass_x, glass_y, available_position` to the UR cobot.
6. **Serve Drink**:  
   The UR cobot picks up the glass from position `(glass_x, glass_y)` and places it at `available_position`.  
   The task is completed after receiving a response from the UR cobot, indicating that the actions have been successfully executed.  
8. **Mark Order as Finished**:  
   Informs the server that the process has been completed.  
   - The finished order is moved from `order_list` to `order_history`.
   - The `serve_order` variable is cleared.  

The parallel structure ensures continuous order reception while drinks are being prepared and served.


## User Interaction Flow

1. **Selecting Ingredients**:
    - Each RFID token corresponds to a drink ingredient.
    - Toss an RFID token into the reader ring to select an ingredient
    - Once a token is read, the corresponding ingredient name is displayed on the LED screen.
    - If more than four ingredients are selected, the extra tokens will be ignored.

2. **Confirming or Clearing Selection**:
    - **Red Button**: Clears the current ingredient selection.
    - **Green Button**: Confirms the order.
    - Upon confirmation, a unique order number is generated and displayed on the LED screen.
    - The system then simulates drink preparation with a 10-second timeout.

3. **Drink Serving**:
    - The UR cobot places the finished drink at an available position in the pickup station.
    - The LED display shows current order numbers at each position (e.g., `0:101, 1: , 2: , 3: `).
    - Once a user picks up the drink, the corresponding position is cleared (e.g., `0: , 1: , 2: , 3: `).
  
## Example Interaction

1. User drops 3 RFID tokens into the reader.  
2. LED screen shows: `Vodka, Lemon Juice, Soda`.  
3. User presses the green button to confirm.  
4. LED shows: `Order number: 102` for 10s.  
5. After the drink is prepared and placed by the UR cobot, the LED updates the serving station status:
   `0:102, 1: , 2: , 3: `  
   (The drink has been placed at position 0).


## Setup Instructions

To run the system:

1. **Connect all hardware components** (RFID reader, LEDs, buttons, weight sensors, etc.).
2. **Power on the UR cobot and start the cobot-side server**. 
3. **On the lehre.bpm server**:  
   SSH into the target machine, then run:
   ```bash
   cd /srv/gruppe/students/ge27zap/endpoint/
   node server.js
   ```
4. **Access the CPEE Process Model**:  
   Launch the Main_process via the following path: /Teaching/Prak/TUM-Prak-24-WS/Jingyi Tang/restructured
