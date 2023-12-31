# Flower_Exchange_LSEG-project-final
## Stock exchange prototype implemented using C++
    Many data structures have been used to test the efficiency and those are in the 'for testing' folder
  
    The finalized program is implemented using Hash Maps, Red black tree containing queues for order book that handles the exchanges

## Main Features
- The program is a MultiThreaded Application with the use of necessary Mutex locks.
- Use of circular buffer by Boost for buffer to exchange data between threads using mutex locks.
- Usage of optimized algorithms and data structures for efficiency
- Adhering to best practices and OOP concepts

## Basic order.csv file example
Order #1
```
Client Order ID,Instrument,Side,Quantity,Price
aa13,,1,100,55.00
aa14,Rose,3,100,65.00
aa15,Lavender,2,101,1.00
aa16,Tulip,1,100,-1.00
aa17,Orchid,1,1000,-1.00
```

Order #2
```
Client Order ID,Instrument,Side,Quantity,Price
aa13,Rose,1,100,55.00
aa14,Rose,1,100,65.00
aa15,Rose,2,300,1.00
```

## Basic execution_rep.csv file example (output)
Execution Report for order #1
```
Client Order ID,Order ID,Instrument,Side,Price,Quantity,Status,Transaction Time
aa13,ord1,,1,55.00,100,Rejected,20230526-194244.228
aa14,ord2,Rose,3,65.00,100,Rejected,20230526-194244.230
aa15,ord3,Lavender,2,1.00,101,Rejected,20230526-194244.230
aa16,ord4,Tulip,1,-1.00,100,Rejected,20230526-194244.230
aa17,ord5,Orchid,1,-1.00,1000,Rejected,20230526-194244.230
```
Execution Report for order #2
```
Client Order ID,Order ID,Instrument,Side,Price,Quantity,Status,Transaction Time
aa13,ord1,Rose,1,55.00,100,New,20230526-194209.790
aa14,ord2,Rose,1,65.00,100,New,20230526-194209.792
aa15,ord3,Rose,2,65.00,100,Pfill,20230526-194209.792
aa14,ord2,Rose,1,65.00,100,Fill,20230526-194209.792
aa15,ord3,Rose,2,55.00,100,Pfill,20230526-194209.792
aa13,ord1,Rose,1,55.00,100,Fill,20230526-194209.792
```
