## Overview

`rfid-cocktail-bot` is a smart, interactive cocktail ordering system that integrates RFID token-based ingredient selection, LED-based information displays, a JavaScript server, and a UR cobot for automated drink delivery. The system logic is orchestrated via the CPEE process engine.

## System Architecture

- **RFID Tokens**: Represent individual drink ingredients.
- **LED Display Module**: Shows selected ingredients, confirmed order number, and the status of drink placements.
- **UR Cobot**: Executes drink delivery actions.
- **JavaScript Server**: Implements and handles CPEE process endpoints, acting as a communication bridge between the CPEE engine and the hardware interface layer.
- **CPEE Process Engine**: Manages the process workflow and communicates with the server via HTTP requests.

## Setup Instructions

To run the system:

1. **Connect all hardware components** (RFID reader, LEDs, buttons, weight sensors, etc.).
2. **Power on the UR robot and start the robot-side server**. 
3. **On the lehre.bpm server**:  
   SSH into the target machine, then run:
   ```bash
   cd /srv/gruppe/students/ge27zap/endpoint/
   node server.js
   ```
4. **Access the CPEE Process Model**:  
   Launch the Main_process via the following path: /Teaching/Prak/TUM-Prak-24-WS/Jingyi Tang/restructured

## User Interaction Flow

1. **Selecting Ingredients**:
    - Each RFID token corresponds to a drink ingredient.
    - Toss an RFID token into the reader ring to select an ingredient
    - Once a token is read, the corresponding ingredient name is displayed on the LED screen.
    - If more than four ingredients are selected, the extra tokens will be ignored.

2. **Managing Selection**:
    - **Red Button**: Clears the current ingredient selection.
    - **Green Button**: Confirms the order.
    - Upon confirmation, a unique order number is generated and displayed on the LED screen.
    - The system then simulates drink preparation with a 10-second timeout.

3. **Drink Serving**:
    - The UR cobot places the finished drink at an available position in the pickup station.
    - The LED display shows current order numbers at each position (e.g., `0:101, 1: , 2: , 3: `).
    - Once a user picks up the drink, the corresponding position is cleared (e.g., `0: , 1: , 2: , 3: `).
