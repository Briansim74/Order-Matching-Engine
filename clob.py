import numpy as np
import pandas as pd
from collections import deque, defaultdict
import os

#input file name and folder
input_file = r"orders-confirmed.csv"
folder = r"C:/Users/admin/Desktop"

#reading the dataframe
df = pd.read_csv(os.path.join(folder, input_file))


def query(specific_ticker, id):

  order_book = defaultdict(lambda: { #create order book with 'Buy' and 'Sell' defaultdicts of deques for queue priority
    'Buy': defaultdict(deque),
    'Sell': defaultdict(deque)
  })

  new_df = df[:id + 1] #get df new up to current id

  for id, row in new_df.iterrows():
    order_type = row['Type']
    side = row['Side']
    ticker = row['Ticker']
    price = float(row['Price'])
    volume = int(row['Volume'])

    # Market Orders
    if order_type == 'M':
      if side == 'Buy': #if market order to buy, sort the current sell orders, lowest price first.
        current_sorted_sells = sorted(order_book[ticker]['Sell'])
        
        for sell_price in current_sorted_sells:
            if volume == 0:
              break #break if we filled all market buys
  
            sell_volume_queue = order_book[ticker]['Sell'][sell_price] #for each sell_price, we get the sell_volume_queue, which is sorted FIFO
            
            while sell_volume_queue and volume > 0: #while current sell_volume_queue is non empty and there is still market buy volume
              
              matched_volume = min(volume, sell_volume_queue[0]) #get appropriate volume to match market buy volume with current sell volume at current group of sell orders
              volume -= matched_volume #reduce market volume
              sell_volume_queue[0] -= matched_volume #reduce current sell volume for top most sell volume

              if sell_volume_queue[0] == 0: #pop front group of sell orders once its been lifted
                sell_volume_queue.popleft()

            if not sell_volume_queue: #delete sell_volume_queue if its empty
              del order_book[ticker]['Sell'][sell_price]

      elif side == 'Sell': #if market order to sell, sort the current buy orders, highest price first.
        current_sorted_buys = sorted(order_book[ticker]['Buy'], reverse = True)
        
        for buy_price in current_sorted_buys:
            if volume == 0:
              break #break if we filled all market sells
            
            buy_volume_queue = order_book[ticker]['Buy'][buy_price] #for each buy_price, we get the buy_volume_queue, which is sorted FIFO
            
            while buy_volume_queue and volume > 0: #while current buy_volume_queue is non empty and there is still market sell volume
              
              matched_volume = min(volume, buy_volume_queue[0]) #get appropriate volume to match market sell volume with current buy volume at current group of buy orders
              volume -= matched_volume #reduce market volume

              buy_volume_queue[0] -= matched_volume #reduce current buy volume for top most buy volume

              if buy_volume_queue[0] == 0: #pop front group of buy orders once its been filled
                buy_volume_queue.popleft()

            if not buy_volume_queue: #delete buy_volume_queue if its empty
              del order_book[ticker]['Buy'][buy_price]

    # Limit Orders
    elif order_type == 'L':
      if side == 'Buy':
        current_sorted_sells = sorted(order_book[ticker]['Sell'])
        
        for sell_price in current_sorted_sells:
            if sell_price > price: #skip sell_price if its higher than buy price for limit orders
              continue

            if volume == 0:
              break #break if we filled all limit buys
  
            sell_volume_queue = order_book[ticker]['Sell'][sell_price] #for each sell_price, we get the sell_volume_queue, which is sorted FIFO
            
            while sell_volume_queue and volume > 0: #while current sell_volume_queue is non empty and there is still limit buy volume
              
              matched_volume = min(volume, sell_volume_queue[0]) #get appropriate volume to match limit buy volume with current sell volume at current group of sell orders
              volume -= matched_volume #reduce limit volume
              sell_volume_queue[0] -= matched_volume #reduce current sell volume for top most sell volume

              if sell_volume_queue[0] == 0: #pop front group of sell orders once its been lifted
                sell_volume_queue.popleft()

            if not sell_volume_queue: #delete sell_volume_queue if its empty
              del order_book[ticker]['Sell'][sell_price]
      
      
      elif side == 'Sell':
        current_sorted_buys = sorted(order_book[ticker]['Buy'], reverse = True)
        
        for buy_price in current_sorted_buys:
            if buy_price < price: #skip buy_price if its lower than sell price for limit orders
                continue

            if volume == 0:
              break #break if we filled all limit sells
  
            buy_volume_queue = order_book[ticker]['Buy'][buy_price] #for each buy_price, we get the buy_volume_queue, which is sorted FIFO
            
            while buy_volume_queue and volume > 0: #while current buy_volume_queue is non empty and there is still limit sell volume
              
              matched_volume = min(volume, buy_volume_queue[0]) #get appropriate volume to match limit sell volume with current buy volume at current group of buy orders
              volume -= matched_volume #reduce limit volume
              buy_volume_queue[0] -= matched_volume #reduce current buy volume for top most buy volume

              if buy_volume_queue[0] == 0: #pop front group of buy orders once its been filled
                buy_volume_queue.popleft()

            if not buy_volume_queue: #delete buy_volume_queue if its empty
              del order_book[ticker]['Buy'][buy_price]


      if volume > 0: #add remaining volume to order book for limit orders
        order_book[ticker][side][price].append(volume)
  
  # Snapshot of order book for specific ticker
  print('Printing OrderBook ----')
  
  # Sells
  current_sorted_sells = sorted(order_book[specific_ticker]['Sell'], reverse = True)
  for sell_price in current_sorted_sells:
    sell_volume_queue = order_book[specific_ticker]['Sell'][sell_price]
    sell_volume = sum(sell_volume_queue)

    print(f'Sell {sell_price} {sell_volume}')

  # Buys
  current_sorted_buys = sorted(order_book[specific_ticker]['Buy'], reverse = True)
  for buy_price in current_sorted_buys:
    buy_volume_queue = order_book[specific_ticker]['Buy'][buy_price]
    buy_volume = sum(buy_volume_queue)

    print(f'Buy {buy_price} {buy_volume}')

  print('End')


while True:
  specific_ticker, id = input("Query: ").split()
  query(int(specific_ticker), int(id))
