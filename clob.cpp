#include <fstream>
#include <iostream>
#include <string>
#include <sstream> // For parsing lines
#include <vector>  // To store extracted data
#include <map> // Red Black Tree for sorted keys
#include <unordered_map>
#include <deque> // Maintain FIFO orders
#include <algorithm>
#include <iomanip>
#include <set>
#include "csv.h" // fast cpp csv parser
using namespace std;

struct Order {
    int id;
    int ticker;
    string action; // "add" or "cancel" order
    string type;   // "limit" or "market"
    string side;   // "buy" or "sell"
    double price;  // For limit orders, or 0 for market
    int volume;
    int cancel_target_id; // cancel order with target_id
};

class OrderBook {
public:
    vector<Order> load_orders_from_csv(const string& filepath, int max_id);
    vector<Order> load_orders_from_csv_with_add_and_cancel(const string& filepath, int max_id); // with add and cancel functionality
    void process_orders(vector<Order>& orders);
    void process_orders_with_add_and_cancel(vector<Order>& orders); // with add and cancel functionality
    void query_ticker(int ticker); // trading ladder format
    void query_ticker_snapshot(int ticker); // default orderbook snapshot format
    void query_pnl();
    void reset();

private:
    unordered_map<int, unordered_map<string, map<double, deque<Order>>>> order_book; // order_book, sorted by: ticker > buy/sell > prices > deques (FIFO)
    unordered_map<int, tuple<int, string,  double>> order_index;
    double pnl = 0; // tracks total pnl, only matched orders realise PnL, cancelled orders do not affect PnL
};


int main(){
    int ticker, max_id;
    OrderBook ob;  // initialize matching engine class
    string filename = "C:\\Users\\admin\\Desktop\\orders-confirmed-with-cancels.csv";
    string filename_with_add_and_cancel = "C:\\Users\\admin\\Desktop\\orders-confirmed-with-cancels.csv";

    while(true){
        cout << "Please enter Ticker and max_id (or -1 -1 to quit): ";
        cin >> ticker >> max_id;

        if (ticker == -1 && max_id == -1) {
            cout << "Exiting query..." << endl;
            return 0;
        }

        // Data with only Add orders
        ob.reset(); // reset order_book

        vector<Order> orders = ob.load_orders_from_csv(filename, max_id); //load orders up to max_id
        ob.process_orders(orders);
        ob.query_pnl();
        ob.query_ticker(ticker);
        // ob.query_ticker_snapshot(ticker);

        // // Data with Add and Cancel orders
        // ob.reset(); // reset order_book

        // vector<Order> orders = ob.load_orders_from_csv_with_add_and_cancel(filename_with_add_and_cancel, max_id); //load orders up to max_id
        // ob.process_orders_with_add_and_cancel(orders);
        // ob.query_pnl();
        // ob.query_ticker(ticker);
        // ob.query_ticker_snapshot(ticker);
    }
    return 0;
}


// using fast cpp csv parser
vector<Order> OrderBook::load_orders_from_csv(const string& filepath, int max_id){
    vector<Order> orders;
    int id, ticker, volume;
    double price;
    string type, side;

    io::CSVReader<6> in(filepath); //set CSVReader to read 6 columns from filepath
    in.read_header(io::ignore_extra_column, "ID", "Ticker", "Type", "Side", "Price", "Volume"); //read header in csv file, ignoring any extra columns, select the 6 headers
    
    while(in.read_row(id, ticker, type, side, price, volume)) { // for each row, select the variables based on same order as read_header

        Order order; // intialize an order struct and assign row variables to order variables
        order.id = id;
        order.ticker = ticker;
        order.type = type;
        order.side = side;
        order.price = price;
        order.volume = volume;

        if(order.id > max_id){
            return orders; // filter orders up to max_id
        }

        orders.push_back(order);
    }
    return orders;
};


// using fast cpp csv parser with add and cancel orders
vector<Order> OrderBook::load_orders_from_csv_with_add_and_cancel(const string& filepath, int max_id){
    vector<Order> orders;
    int id, ticker, volume, cancel_target_id;
    double price;
    string action, type, side;

    io::CSVReader<8> in(filepath); //set CSVReader to read 8 columns from filepath
    in.read_header(io::ignore_extra_column, "ID", "Ticker", "Action", "Type", "Side", "Price", "Volume", "Cancel_Target_ID"); //read header in csv file, ignoring any extra columns, select the 6 headers

    while(in.read_row(id, ticker, action, type, side, price, volume, cancel_target_id)) { // for each row, select the variables based on same order as read_header

        Order order; // intialize an order struct and assign row variables to order variables
        order.id = id;
        order.ticker = ticker;
        order.action = action;
        order.type = type;
        order.side = side;
        order.price = price;
        order.volume = volume;
        order.cancel_target_id = cancel_target_id;

        if(order.id > max_id){
            return orders; // filter orders up to max_id
        }

        orders.push_back(order);
    }
    return orders;
};


// Process order_book
void OrderBook::process_orders(vector<Order>& orders){
    for (auto& order : orders) {// match & insert each order into book

        // Market Orders
        if(order.type == "M"){
            if(order.side == "Buy"){ // if market order to buy, sort the current sell orders, lowest price first.
                vector<double> sell_prices_to_delete;

                for(auto& [sell_price, sell_volume_queue]: order_book[order.ticker]["Sell"]){ //iterate through all the sell prices, starting from lowest to highest
                    if(order.volume == 0){
                        break; // break if we filled all market buys
                    }

                    while(!sell_volume_queue.empty() && order.volume > 0){ // while current sell_volume_queue is non empty and there is still market buy volume

                        int matched_volume = min(order.volume, sell_volume_queue.front().volume); // get appropriate volume to match market buy volume with current sell volume at current group of sell orders
                        order.volume -= matched_volume; // reduce market volume
                        sell_volume_queue.front().volume -= matched_volume; // reduce current sell volume for top most sell volume

                        pnl += matched_volume * sell_price; //track pnl, lifting orders, gaining cash

                        if(sell_volume_queue.front().volume == 0){ // pop front group of sell orders once its been lifted
                            sell_volume_queue.pop_front();
                        }
                    }

                    if(sell_volume_queue.empty()){ // add sell_price to sell_prices_to_delete if the deque is empty
                        sell_prices_to_delete.push_back(sell_price);
                    }
                }

                for(const auto& sell_price: sell_prices_to_delete){ // delete all sell_prices that have empty deques
                    order_book[order.ticker]["Sell"].erase(sell_price);
                }
            }

            else if(order.side == "Sell"){
                vector<double> buy_prices_to_delete;

                for(auto it = order_book[order.ticker]["Buy"].rbegin(); it != order_book[order.ticker]["Buy"].rend(); ++it) { //reverse iterate through all the buy prices, starting from highest to lowest
                    auto& buy_price = it->first;
                    auto& buy_volume_queue = it->second;
                    
                    if(order.volume == 0){
                        break; // break if we filled all market sells
                    }

                    while(!buy_volume_queue.empty() && order.volume > 0){ // while current buy_volume_queue is non empty and there is still market buy volume
                        
                        int matched_volume = min(order.volume, buy_volume_queue.front().volume); // get appropriate volume to match market buy volume with current buy volume at current group of buy orders
                        order.volume -= matched_volume; // reduce market volume
                        buy_volume_queue.front().volume -= matched_volume; // reduce current buy volume for top most buy volume

                        pnl -= matched_volume * buy_price; //track pnl, filling orders, spending cash

                        if(buy_volume_queue.front().volume == 0){ // pop front group of buy orders once its been filled
                            buy_volume_queue.pop_front();
                        }
                    }

                    if(buy_volume_queue.empty()){ // add buy_price to buy_prices_to_delete if the deque is empty
                        buy_prices_to_delete.push_back(buy_price);
                    }
                }

                for(const auto& buy_price: buy_prices_to_delete){ // delete all sell_prices that have empty deques
                    order_book[order.ticker]["Buy"].erase(buy_price);
                }
            }
        }

        // Limit Orders
        else if(order.type == "L"){
            if(order.side == "Buy"){
                vector<double> sell_prices_to_delete;

                for(auto& [sell_price, sell_volume_queue]: order_book[order.ticker]["Sell"]){ //iterate through all the sell prices, starting from lowest to highest
                    if(sell_price > order.price){
                        continue; // skip sell_price if its higher than buy price for limit orders
                    }
                    
                    if(order.volume == 0){
                        break; // break if we filled all market buys
                    }

                    while(!sell_volume_queue.empty() && order.volume > 0){ // while current sell_volume_queue is non empty and there is still market buy volume
                        
                        int matched_volume = min(order.volume, sell_volume_queue.front().volume); // get appropriate volume to match market buy volume with current sell volume at current group of sell orders
                        order.volume -= matched_volume; // reduce market volume
                        sell_volume_queue.front().volume -= matched_volume; // reduce current sell volume for top most sell volume

                        pnl += matched_volume * sell_price; //track pnl, lifting orders, gaining cash

                        if(sell_volume_queue.front().volume == 0){ // pop front group of sell orders once its been lifted
                            sell_volume_queue.pop_front();
                        }
                    }

                    if(sell_volume_queue.empty()){ // add sell_price to sell_prices_to_delete if the deque is empty
                        sell_prices_to_delete.push_back(sell_price);
                    }
                }

                for(const auto& sell_price: sell_prices_to_delete){ // delete all sell_prices that have empty deques
                    order_book[order.ticker]["Sell"].erase(sell_price);
                }
            }

            else if(order.side == "Sell"){
                vector<double> buy_prices_to_delete;

                for(auto it = order_book[order.ticker]["Buy"].rbegin(); it != order_book[order.ticker]["Buy"].rend(); ++it) { //reverse iterate through all the buy prices, starting from highest to lowest
                    auto& buy_price = it->first;
                    auto& buy_volume_queue = it->second;

                    if(buy_price < order.price){
                        continue; // skip buy_price if its lower than sell price for limit orders
                    }
                    
                    if(order.volume == 0){
                        break; // break if we filled all market sells
                    }

                    while(!buy_volume_queue.empty() && order.volume > 0){ // while current buy_volume_queue is non empty and there is still market buy volume
                        
                        int matched_volume = min(order.volume, buy_volume_queue.front().volume); // get appropriate volume to match market buy volume with current buy volume at current group of buy orders
                        order.volume -= matched_volume; // reduce market volume
                        buy_volume_queue.front().volume -= matched_volume; // reduce current buy volume for top most buy volume

                        pnl -= matched_volume * buy_price; //track pnl, filling orders, spending cash

                        if(buy_volume_queue.front().volume == 0){ // pop front group of buy orders once its been filled
                            buy_volume_queue.pop_front();
                        }
                    }

                    if(buy_volume_queue.empty()){ // add buy_price to buy_prices_to_delete if the deque is empty
                        buy_prices_to_delete.push_back(buy_price);
                    }
                }

                for(const auto& buy_price: buy_prices_to_delete){ // delete all sell_prices that have empty deques
                    order_book[order.ticker]["Buy"].erase(buy_price);
                }
            }

            if(order.volume > 0){ // add remaining volume to order book for limit orders
                order_book[order.ticker][order.side][order.price].push_back(order);
            }
        }
    }
}


// Process order_book with add and cancel orders
void OrderBook::process_orders_with_add_and_cancel(vector<Order>& orders){
    for (auto& order : orders) {// match & insert each order into book

        // Adding Orders
        if(order.action == "Add"){
            
            // Market Orders
            if(order.type == "M"){
                if(order.side == "Buy"){ // if market order to buy, sort the current sell orders, lowest price first.
                    vector<double> sell_prices_to_delete;

                    for(auto& [sell_price, sell_volume_queue]: order_book[order.ticker]["Sell"]){ //iterate through all the sell prices, starting from lowest to highest
                        if(order.volume == 0){
                            break; // break if we filled all market buys
                        }

                        while(!sell_volume_queue.empty() && order.volume > 0){ // while current sell_volume_queue is non empty and there is still market buy volume

                            int matched_volume = min(order.volume, sell_volume_queue.front().volume); // get appropriate volume to match market buy volume with current sell volume at current group of sell orders
                            order.volume -= matched_volume; // reduce market volume
                            sell_volume_queue.front().volume -= matched_volume; // reduce current sell volume for top most sell volume

                            pnl += matched_volume * sell_price; //track pnl, lifting orders, gaining cash

                            if(sell_volume_queue.front().volume == 0){ // pop front group of sell orders once its been lifted
                                sell_volume_queue.pop_front();
                            }
                        }

                        if(sell_volume_queue.empty()){ // add sell_price to sell_prices_to_delete if the deque is empty
                            sell_prices_to_delete.push_back(sell_price);
                        }
                    }

                    for(const auto& sell_price: sell_prices_to_delete){ // delete all sell_prices that have empty deques
                        order_book[order.ticker]["Sell"].erase(sell_price);
                    }
                }

                else if(order.side == "Sell"){
                    vector<double> buy_prices_to_delete;

                    for(auto it = order_book[order.ticker]["Buy"].rbegin(); it != order_book[order.ticker]["Buy"].rend(); ++it) { //reverse iterate through all the buy prices, starting from highest to lowest
                        auto& buy_price = it->first;
                        auto& buy_volume_queue = it->second;
                        
                        if(order.volume == 0){
                            break; // break if we filled all market sells
                        }

                        while(!buy_volume_queue.empty() && order.volume > 0){ // while current buy_volume_queue is non empty and there is still market buy volume
                            
                            int matched_volume = min(order.volume, buy_volume_queue.front().volume); // get appropriate volume to match market buy volume with current buy volume at current group of buy orders
                            order.volume -= matched_volume; // reduce market volume
                            buy_volume_queue.front().volume -= matched_volume; // reduce current buy volume for top most buy volume

                            pnl -= matched_volume * buy_price; //track pnl, filling orders, spending cash

                            if(buy_volume_queue.front().volume == 0){ // pop front group of buy orders once its been filled
                                buy_volume_queue.pop_front();
                            }
                        }

                        if(buy_volume_queue.empty()){ // add buy_price to buy_prices_to_delete if the deque is empty
                            buy_prices_to_delete.push_back(buy_price);
                        }
                    }

                    for(const auto& buy_price: buy_prices_to_delete){ // delete all sell_prices that have empty deques
                        order_book[order.ticker]["Buy"].erase(buy_price);
                    }
                }
            }

            // Limit Orders
            else if(order.type == "L"){
                if(order.side == "Buy"){
                    vector<double> sell_prices_to_delete;

                    for(auto& [sell_price, sell_volume_queue]: order_book[order.ticker]["Sell"]){ //iterate through all the sell prices, starting from lowest to highest
                        if(sell_price > order.price){
                            continue; // skip sell_price if its higher than buy price for limit orders
                        }
                        
                        if(order.volume == 0){
                            break; // break if we filled all market buys
                        }

                        while(!sell_volume_queue.empty() && order.volume > 0){ // while current sell_volume_queue is non empty and there is still market buy volume
                            
                            int matched_volume = min(order.volume, sell_volume_queue.front().volume); // get appropriate volume to match market buy volume with current sell volume at current group of sell orders
                            order.volume -= matched_volume; // reduce market volume
                            sell_volume_queue.front().volume -= matched_volume; // reduce current sell volume for top most sell volume

                            pnl += matched_volume * sell_price; //track pnl, lifting orders, gaining cash

                            if(sell_volume_queue.front().volume == 0){ // pop front group of sell orders once its been lifted
                                sell_volume_queue.pop_front();
                            }
                        }

                        if(sell_volume_queue.empty()){ // add sell_price to sell_prices_to_delete if the deque is empty
                            sell_prices_to_delete.push_back(sell_price);
                        }
                    }

                    for(const auto& sell_price: sell_prices_to_delete){ // delete all sell_prices that have empty deques
                        order_book[order.ticker]["Sell"].erase(sell_price);
                    }
                }

                else if(order.side == "Sell"){
                    vector<double> buy_prices_to_delete;

                    for(auto it = order_book[order.ticker]["Buy"].rbegin(); it != order_book[order.ticker]["Buy"].rend(); ++it) { //reverse iterate through all the buy prices, starting from highest to lowest
                        auto& buy_price = it->first;
                        auto& buy_volume_queue = it->second;

                        if(buy_price < order.price){
                            continue; // skip buy_price if its lower than sell price for limit orders
                        }
                        
                        if(order.volume == 0){
                            break; // break if we filled all market sells
                        }

                        while(!buy_volume_queue.empty() && order.volume > 0){ // while current buy_volume_queue is non empty and there is still market buy volume
                            
                            int matched_volume = min(order.volume, buy_volume_queue.front().volume); // get appropriate volume to match market buy volume with current buy volume at current group of buy orders
                            order.volume -= matched_volume; // reduce market volume
                            buy_volume_queue.front().volume -= matched_volume; // reduce current buy volume for top most buy volume

                            pnl -= matched_volume * buy_price; //track pnl, filling orders, spending cash

                            if(buy_volume_queue.front().volume == 0){ // pop front group of buy orders once its been filled
                                buy_volume_queue.pop_front();
                            }
                        }

                        if(buy_volume_queue.empty()){ // add buy_price to buy_prices_to_delete if the deque is empty
                            buy_prices_to_delete.push_back(buy_price);
                        }
                    }

                    for(const auto& buy_price: buy_prices_to_delete){ // delete all sell_prices that have empty deques
                        order_book[order.ticker]["Buy"].erase(buy_price);
                    }
                }

                if(order.volume > 0){ // add remaining volume to order book for limit orders
                    order_book[order.ticker][order.side][order.price].push_back(order);
                    order_index[order.id] = make_tuple(order.ticker, order.side, order.price);
                }
            }
        }

        // Cancelling existing orders
        else if(order.action == "Cancel"){
            if(order_index.find(order.cancel_target_id) == order_index.end()){
                cout << "Cancel_Target_Id " << order.cancel_target_id << " not found, skipping to next order..." << endl;
                continue;
            }

            auto& [ticker, side, price] = order_index[order.cancel_target_id]; //unordered_map<int, tuple<int, string,  double>> order_index;
            auto& volume_queue = order_book[ticker][side][price];

            for(auto existing_order = volume_queue.begin(); existing_order != volume_queue.end(); ++existing_order){
                if(existing_order->id == order.cancel_target_id){
                    volume_queue.erase(existing_order); // erase existing order from volume_queue deque
                    break;
                }
            }

            order_index.erase(order.cancel_target_id); // erase key from order_index after cancellation

            if(volume_queue.empty()){
                order_book[ticker][side].erase(price); // erase price in order_book if whole deque is empty after cancellation
            }
        }
    }
}


// Trading ladder format
void OrderBook::query_ticker(int ticker){ // Snapshot of order book for specific ticker
    cout << "Ticker: " << ticker << endl;
    cout << "Bid Size | Price  | Ask Size" << endl;
    cout << "---------+--------+---------" << endl;

    set<double, greater<double>> prices; // Create a descending set of outstanding prices of orders, as there may be duplicate prices for both buys and sells

    for(auto& [price, sell_volume_queue]: order_book[ticker]["Sell"]){
        prices.insert(price);
    }

    for(auto& [price, buy_volume_queue]: order_book[ticker]["Buy"]){
        prices.insert(price);
    }

    for(double price: prices){
        int buy_volume = 0;
        if(order_book[ticker]["Buy"].count(price)){ // Sum buy volume if price exists on Buy side
            for(const auto& order: order_book[ticker]["Buy"][price]){
                buy_volume += order.volume;
            }
        }

        int sell_volume = 0;
        if(order_book[ticker]["Sell"].count(price)){ // Sum sell volume if price exists on Sell side
            for(const auto& order: order_book[ticker]["Sell"][price]){
                sell_volume += order.volume;
            }
        }

        cout << setw(7); // Set constant width of Buy side column
        if(buy_volume > 0){
            cout << buy_volume;
        }
        else cout << " ";

        cout << "  | " << setw(6) << price << " | "; // Set constant width of Price column
        
        if(sell_volume > 0){
            cout << sell_volume;
        }
        else cout << " ";
        cout << endl;
    }
}


// Snapshot format
void OrderBook::query_ticker_snapshot(int ticker){ // Snapshot of order book for specific ticker
    cout << "Printing OrderBook ----" << endl;

    // Sells
    for(auto it = order_book[ticker]["Sell"].rbegin(); it != order_book[ticker]["Sell"].rend(); ++it){ // Printing sells from highest to lowest
        auto& sell_price = it->first;
        auto& sell_volume_queue = it->second;

        int sell_volume = 0;
        for(const auto& order: sell_volume_queue){
            sell_volume += order.volume;
        }

        cout << "Sell " << sell_price << " " << sell_volume << endl;
    }

    // Buys
    for(auto it = order_book[ticker]["Buy"].rbegin(); it != order_book[ticker]["Buy"].rend(); ++it){ // Printing buys from highest to lowest
        auto& buy_price = it->first;
        auto& buy_volume_queue = it->second;

        int buy_volume = 0;
        for(const auto& order: buy_volume_queue){
            buy_volume += order.volume;
        }

        cout << "Buy " << buy_price << " " << buy_volume << endl;
    }

    cout << "End" << endl;
}


// Query PnL
void OrderBook::query_pnl(){
    cout << endl << fixed << setprecision(2) <<  "Total PnL: $" << pnl << endl << endl;
}


// Reset order_book class
void OrderBook::reset(){
    order_book.clear(); // Clear entire order_book
    pnl = 0; // reset PnL
}


// // using fstream and sstream
// vector<Order> OrderBook::load_orders_from_csv(const string& filepath, int max_id){
//     string line;
//     ifstream file(filepath);
//     vector<Order> orders;
//     string cell; //each column value in each line, eg "ID", Ticker", "Side", "Type", "Price", "Volume"
    
//     if (!file.is_open()) {
//         cerr << "Error opening file!" << endl;
//     }

//     getline(file, line); // Skip header row
    
//     while(getline(file, line)) {
//         stringstream ss(line); // converts each line string / row into a stringstream object, can skip this and do it in a normal vector loop
//         string cell; //each column value in each line, eg "ID", Ticker", "Side", "Type", "Price", "Volume"
//         vector<string> row_data;

//         while(getline(ss, cell, ',')) { // Now 'row_data' contains all the cell values for the current row
//             row_data.push_back(cell);
//         }

//         Order order; // intialize an order struct
//         order.id = stoi(row_data[0]); //convert string to int
//         order.ticker = stoi(row_data[1]);
//         order.type = row_data[2];
//         order.side = row_data[3];
//         order.price = stod(row_data[4]); //convert string to double
//         order.volume = stoi(row_data[5]);

//         if(order.id > max_id){
//             return orders; //filter orders up to max_id
//         }

//         orders.push_back(order);

//         // for (const auto& value : row_data) { // You can process or store 'row_data' as needed, e.g., print them
//         //     cout << value << "\t";
//         // }
//         // cout << endl;
//     }
//     return orders;
// };