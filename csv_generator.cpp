#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <random>
#include <iomanip> // set double precision
using namespace std;

void generate_csv(string filename){

    // Initialize parameters
    int rows = 100000;
    float cancel_rate = 0.5; // Cancel vs Add rate
    float limit_order_rate = 0.7; // Limit vs Market Order rate

    vector<int> cancel_target_ids_array; // array to store existing ids for cancels
    vector<int> tickers = {1131, 2211, 2313};
    vector<string> actions = {"Add", "Cancel"};
    vector<string> types = {"Market", "Limit"};
    vector<string> sides = {"Buy", "Sell"};

    // Setup random engines
    random_device rd;
    mt19937 gen(rd());
    uniform_real_distribution<> action_dis(0.0, 1.0); // for deciding Add/Cancel
    uniform_int_distribution<> ticker_dis(0, tickers.size() - 1); // Random tickers
    uniform_real_distribution<> type_dis(0.0, 1.0); // for deciding Market/Limit Orders
    uniform_int_distribution<> side_dis(0, sides.size() - 1); // Random sides
    uniform_real_distribution<double> price_dis(40.0, 238.4); // Random prices
    uniform_int_distribution<int> volume_dis(1, 590); // Random volumes
    
     // Create and open the CSV file
    ofstream outfile(filename);

    // Write CSV header and set precision to 2 d.p.
    outfile << "ID,Ticker,Action,Type,Side,Price,Volume,Cancel_Target_ID" << endl;
    outfile << fixed << setprecision(2);

    for(int id = 0; id < rows; id++){
        cout << "ID: " << id << endl;

        float add_or_cancel = action_dis(gen); // random value between 0.0 to 1.0 to determine if number is below the cutoff cancel_rate

        // Cancel
        if(add_or_cancel < cancel_rate && !cancel_target_ids_array.empty()){ // if action is a cancel and there are existing ids to cancel
            
            // Update random cancel_target_ids distribution with array size each loop
            uniform_int_distribution<> cancel_target_id_dis(0, cancel_target_ids_array.size() - 1);

            // Generate random target id to cancel
            int cancel_target_id = cancel_target_ids_array[cancel_target_id_dis(gen)];
            
            // id, ticker, action, type, side, price, volume, cancel_target_id
            outfile << id << ",-1,Cancel,-1,-1,-1,-1," << cancel_target_id << endl;
            
            // Remove target id from cancel_target_ids_array
            cancel_target_ids_array.erase(find(cancel_target_ids_array.begin(), cancel_target_ids_array.end(), cancel_target_id));
        }

        // Add
        else{
            // Generate random variables
            int ticker = tickers[ticker_dis(gen)]; // random ticker
            float limit_or_market = type_dis(gen); // random value between 0.0 to 1.0 to determine if number is below the cutoff limit_order_rate
            string side = sides[side_dis(gen)]; // random side
            double price = price_dis(gen); // random double price
            int volume = volume_dis(gen); // random int volume

            // Limit Order
            // id, ticker, action, type, side, price, volume, cancel_target_id
            if(limit_or_market < limit_order_rate){
                
                // Add current id to cancel_target_ids_array, only for limit orders
                cancel_target_ids_array.push_back(id);

                outfile << id << "," << ticker << ",Add,L," << side << "," << price << "," << volume << ",-1" << endl;
            }

            // Market Order
            else{
                outfile << id << "," << ticker << ",Add,M," << side << ",-1," << volume << ",-1" << endl;
            }
        }
    }
    outfile.close(); // Close the file when done
}


int main(){
    string filename = "C:\\Users\\admin\\Desktop\\orders-confirmed-with-cancels.csv";
    generate_csv(filename);

    return 0;
}