# Website Load Balancer using Weighted Fair Queuing (WFQ) Algorithm


This C++ project implements a Website Load Balancer using the Weighted Fair Queuing (WFQ) algorithm. It simulates the distribution of HTTP requests across multiple websites based on their allocated bandwidth and processing power. This load balancer aims to ensure fairness in handling requests while efficiently utilizing available resources.

This project demonstrates a Website Load Balancer implemented in C++. It includes classes for HttpRequest, Website, and LoadBalancer, along with functionalities to enqueue and dequeue requests using the WFQ algorithm.

Features
Addition of websites with specified attributes (website ID, owner ID, bandwidth, processing power).
Enqueuing HTTP requests into the corresponding website's queue.
Dequeuing requests based on the Weighted Fair Queuing (WFQ) scheduling algorithm.
Simulated processing and rebalancing of requests across websites.
