# Order Matching Engine with Cancel & PnL Functionality
A fast, extensible Central Limit Order Book (CLOB) simulation written in C++ and Python, capable of processing Market and Limit Orders, tracking Profit & Loss (PnL), and handling cancelled orders. You can query snapshots of the order book at any time to inspect the state of outstanding orders for specific tickers.

## Project Overview
This project simulates a Central Limit Order Book (CLOB), which most modern exchanges use to match buy and sell orders. The engine maintains a live book of outstanding orders and matches them using price-time priority.

## This repository includes:
- A C++ order matching engine with cancel order and PnL tracking support
- A Python version for prototyping and conceptual understanding
- A C++ CSV generator to create randomized order flows with cancel logic

## Features

| Feature                | Description                                                                      |
| -----------------------|----------------------------------------------------------------------------------|
| Limit & Market Orders  | Supports both limit and market order types                                       |
| FIFO Matching          | Orders at the same price level are matched by time priority (First-In-First-Out) |
| Cancel Orders          | Cancel outstanding limit orders by referencing order ID                          |
| PnL Tracking           | Tracks cumulative PnL from matched trades                                        |
| CSV Order Generator    | Generates randomized test data with cancel logic                                 |
| Snapshot Querying      | View order book state at any time for a specific ticker                          |
| C++ & Python           | Dual implementation for learning and conceptual understanding                    |

## Project Structure
- clob.cpp (C++ Order Matching Engine)
- clob.exe (Compiled C++ Executable)
- clob.py  (Python Order Matching Engine)
- csv_generator.cpp (CSV order generator)
- csv_generator.exe (Compiled C++ Executive)
- orders-confirmed.csv (Sample order flow csv - Add only)
- orders-confirmed-with-cancels.csv (Sample order flow csv - Add and Cancels)
- csv.h (Fast C++ csv parser library for parsing csv inputs)

## How it works

### Order Matching Logic
The engine maintains an order_book, structured as:

```cpp
unordered_map<int, unordered_map<string, map<double, deque<Order>>>>
               ^                    ^           ^           ^
             ticker                side       price    order queue (FIFO)
```

- Buy orders match the lowest sell price first
- Sell orders match the highest buy price first
- Market orders are immediate or cancel (IOC)
- Limit orders match if possible, otherwise, they are added to the book

## Cancel Orders
Limit orders can be cancelled using their unique order ID. When a cancel order is processed, the engine:
- Finds the corresponding price level and side
- Removes the order from the FIFO queue
- Deletes empty queues to keep the book clean

## PnL Calculation
PnL is tracked as:
- Positive for market buys (Lifting Offers)
- Negative for market sells (Hitting Bids)

Cancelled orders are not included in the PnL calculation as they are never matched.

## Getting Started
1. Compile the C++ engine

g++ -std=c++17 -O2 -o engine clob.cpp

2. Run the Engine

./clob.exe

You'll be prompted:

Please enter Ticker and max_id (or -1 -1 to quit):

- Ticker: Numeric ticker symbol (e.g. 1131)
- max_id: Maximum order ID to process (e.g. 42321)

### Modes of Operation
The engine supports two modes depending on how you want the order data to be processed:

### Basic Mode (Add-Only orders) - Default
This is the active code in the main() function, and processes only Add orders, using orders-confirmed.csv. Additionally, the traditional snapshot format (ob.query_ticker_snapshot(ticker)) is commented out, and currently the standard trading ladder format (ob.query_ticker(ticker)) is used. This can also be uncommented to view the other format.

```cpp
// Data with only Add orders
ob.reset(); // reset order_book

vector<Order> orders = ob.load_orders_from_csv(filename, max_id); //load orders up to max_id
ob.process_orders(orders);
ob.query_pnl();
ob.query_ticker(ticker);
// ob.query_ticker_snapshot(ticker);
```

This is useful for:
- Debugging core matching logic
- Understanding basic PnL behaviour

### Cancel-Enabled Mode (Add & Cancel Orders)
This can be enabled by uncommenting the block near the bottom of main():

```cpp
// // Data with Add and Cancel orders
// ob.reset(); // reset order_book

// vector<Order> orders = ob.load_orders_from_csv_with_add_and_cancel(filename_with_add_and_cancel, max_id); //load orders up to max_id
// ob.process_orders_with_add_and_cancel(orders);
// ob.query_pnl();
// ob.query_ticker(ticker);
// ob.query_ticker_snapshot(ticker);
```

In this mode, the engine handles:
- Market & Limit orders (Add)
- Cancel Limit orders (By ID)

This mode is ideal for:
- Simulating real exchange behaviour
- Validating cancel logic
- Accurate PnL tracking

### Alternative CSV parsing via fstream and sstream
An alternative function using fstream and sstream is commented at the bottom of the code, instead of using the fast cpp csv parser library.

## Python Version (For Prototyping)
The Python version (clob.py) provides a simplified version of the matching engine logic, using defaultdict and deque to simulate price-time priority. Simple for visualization and concept validation but not optimized for speed.

## CSV Generator
The CSV generator (csv_generator.cpp) creates randomized synthetic order flow data to test the matching engine. It supports both Add and Cancel orders, with customizable paramaters for:
- Number of orders
- Cancel Rate
- Limit vs Market Order Ratio
- Random tickers, sides, prices, and volumes

The generated CSV file is compatible with the C++ matching engine with Cancel-Enabled Mode. Placeholder values of -1 are filled for N/A cells.

## Features

| Feature                       | Description                                                                  |
| ------------------------------|------------------------------------------------------------------------------|
| Number of Orders              | Specify the size of the input flow data                                      |
| Cancel Rate                   | Adjustable Add-to-Cancel rates                                               |
| Limit vs Market Order Ratio   | Adjustable Limit-t0-Market order rates                                       |
| Randomized Orders             | Random generation of ticker, side, price and volume                          |
| ID Tracking Logic             | Maintains a dynamic array of valid Limit order IDs for cancellation          |
| Cancel Order Support          | Cancel outstanding limit orders by referencing order ID                      |

## Sample CSV Output Format
```csv
ID	Ticker	Action	Type	Side	Price	Volume	Cancel_Target_ID
0	2211	Add	M	Sell	-1	402	-1
1	1131	Add	M	Sell	-1	380	-1
2	1131	Add	L	Buy	76.03	69	-1
3	1131	Add	M	Buy	-1	558	-1
4	-1	Cancel	-1	-1	-1	-1	2
5	2313	Add	M	Buy	-1	138	-1
```

| Field                         | Description                                                                  |
| ------------------------------|------------------------------------------------------------------------------|
| ID                            | Unique order ID                                                              |
| Ticker                        | Numeric ticker symbol                                                        |
| Action                        | "Add" or "Cancel"                                                            |
| Type                          | "Limit" or "Market"                                                          |
| Side                          | "Buy" or "Sell"                                                              |
| Price                         | -1 for Market Orders                                                         |
| Volume                        | Order size                                                                   |
| Cancel_Target_ID              | ID of the limit order to cancel, -1 if not a cancel                          |

## Example Outputs
Here is how the engine behaves during a typical query session:

### C++ (Add-Only Orders)
### C++ Entering Query
<img width="412" height="28" alt="Input Query C++" src="https://github.com/user-attachments/assets/ebb46dbd-4e9c-4c48-8512-7db8c402aacd" />

### With trading ladder view
<img width="409" height="511" alt="Trading Ladder View (Add-Only)" src="https://github.com/user-attachments/assets/3e34ba41-6152-47d9-a9f7-f68f4abfd972" />

### With snapshot view
<img width="405" height="496" alt="Snapshot View (Add-Only)" src="https://github.com/user-attachments/assets/b79f296f-72bd-45a8-8e1c-48120d1b5396" />

### C++ (Add and Cancel Orders)
<img width="396" height="233" alt="Trading Ladder View (Add and Cancel)" src="https://github.com/user-attachments/assets/d1950ba5-2b0b-45ad-a295-1efdb3bc18de" />

### C++ Exiting Query
<img width="745" height="62" alt="Exiting Query C++" src="https://github.com/user-attachments/assets/dcc8790a-81f4-4bd0-8446-a6d57e49668e" />

### Python (Add-Only Orders)
<img width="181" height="354" alt="Python Query (Add-Only)" src="https://github.com/user-attachments/assets/bf658e0c-fd1c-418f-a4d6-9f80d7f80822" />

### Example CSV Generation
<img width="152" height="253" alt="image" src="https://github.com/user-attachments/assets/52bdb13e-576b-488d-a909-98cec89fdd1d" />
