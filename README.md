# Order Matching Engine
Query Snapshot of Central Limit Order Book within the Market Order Matching Engine


In this notebook, I will be demonstrating the workings of the Order Matching Engine. The engine manages the Central Limit Order Book for Market and Limit Orders. Snapshots can be queried for specific tickers to view current outstanding ticker orders.

clob.ipynb

Firstly, I will be importing a csv file with the following columns:

ID
Ticker
Type - ('Market Order' / 'Limit Order')
Side - ('Buy' / 'Sell')
Price
Volume (Trading Volume)

Most exchanges around the world use a central limit order book. In a typical market, buyers and sellers must submit orders to a central limit order book, which aggregates and matches all outstanding buy and sell orders. Orders are submitted to an order book and are filled according to price-time priority; matched first by best price followed by time(first in first out). If they are not filled immediately, they can become part of the order book which records all the buy and sell orders at different price levels. 

There are two main order types in most CLOB and we will be exploring them today. They are Market Orders and Limit Orders.

Market Orders
Market orders execute at the best possible price of the instrument with the purpose of filling the entire volume of the order. If an order book receives a market order to buy X volume of instrument Y, the order will be matched by sell orders in the order book by price-time priority until the X volume is fully filled or if there are no more sell orders available. If the market order cannot be fully filled, the remaining order will be killed, and it will not reside in the order book.

Limit Orders
Limit orders are submitted with a specified maximum price to be paid or minimum price to be received for a given volume of an instrument. Limit order to buy will be filled by sell orders on the order book with price less than or equal to the specified limit price. Limit order to sell will be filled by buy orders on the order book with price greater than or equal to the specified limit price. The limit order will be filled by price-time priority. If the limit order is not fully filled, it will remain on the order book at the specified limit price waiting to be filled by future orders.

In my code, I will be querying a specific ticker (e.g. 1111) by parsing all orders up to the specific ID. Then, a snapshot of the outstanding orders of the 1111 tickers on the order book would be printed.

The code can be split up into 2 major parts: Market & Limit Orders. I will first create an order_book, which is a defaultdict comprised of 'Buy' and 'Sell'. Within each 'Buy' or 'Sell' key, will be individual defaultdicts, where the key is the price of the order, and the value would house a deque full of volume orders, ordered in FIFO (First-In-First-Out). This is to allow us to place price and time priority for specific prices of orders. Buy orders will try to be first matched to the lowest ask price, and ask orders will try to be first to the highest bid price. (E.g. ID 2 for ticker 2313 'Buy' 150 @ 73.4 vs ID 7 for ticker 2313 'Buy' 200 @ 73.4. ID 2 will be placed in a higher priority than ID 7 due to FIFO at that specific trading price.)

While iterating through the dataframe, market orders will be filled and killed (Immediate or Cancel Order). For a buy, the current lowest ask and earliest order will be filled first. Vice versa to a sell.

For limit orders, the current limit order would first check if it meets criteria for immediate trading. For a buy, the lowest ask will be compared against the buy price of the limit order. Orders will be filled until ask price exceeds limit buy price. Vice versa for a sell. Afterwards, remaining orders would be added to the order_book.

Finally, a snapshot for the specific ticker will be requested, where outstanding orders are sorted from highest to lowest price.

Example queries can be seen.
