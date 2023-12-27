#include <iostream>
#include <vector>
#include <queue>
#include <chrono>
#include <thread>

#ifdef __unix__
#include <unistd.h>
#elif defined _WIN32
#include <windows.h>
#define sleep(x) Sleep(1000 * (x))
#endif

int TOTAL_BANDWIDTH=30;

using namespace std;

class HttpRequest {
    int requestID;
    int websiteID;
    int processingTime;

public:
    HttpRequest(int requestID, int websiteID, int processingTime)
    : requestID(requestID), websiteID(websiteID), processingTime(processingTime) {}

    int getRequestID() const {
        return requestID;
    }

    int getWebsiteID() const {
        return websiteID;
    }

    int getProcessingTime() const {
        return processingTime;
    }
};

class Website {
    int websiteID;
    int ownerID;
    int bandwidth;
    int processingPower;

public:
    Website(int websiteID, int ownerID, int bandwidth, int processingPower)
    : websiteID(websiteID), ownerID(ownerID), bandwidth(bandwidth), processingPower(processingPower) {}

    int getWebsiteID() const {
        return websiteID;
    }

    int getOwnerID() const {
        return ownerID;
    }

    int getBandwidth() const {
        return bandwidth;
    }

    int getProcessingPower() const {
        return processingPower;
    }
};

class LoadBalancer {
public:
    // Define data structures for websites, queues, and WFQ
    vector<Website> websites;
    vector<queue<HttpRequest>> requestQueues; // requests queue for each website
    vector<double> websiteWeights; // Store weights for each website
    vector<double> websiteDummyWeights;
    vector<double> websiteDummyWeights2;
    vector<int> website_round;

    int sum1 = 0;
    int sum2 = 0;
    int sum3 = 0;
    int count = 0;
    int count_while = 0;

    LoadBalancer() {}

    void addWebsite(int websiteID, int ownerID, int bandwidth, int processingPower) {
        // Add a new website with the specified attributes
        // Update data structures for WFQ
        int x = 0;
        websites.push_back(Website(websiteID, ownerID, bandwidth, processingPower));
        requestQueues.push_back(queue<HttpRequest>());

        // Update website weights based on allocated resources
        websiteWeights.push_back(static_cast<double>(bandwidth + processingPower));
        websiteDummyWeights.push_back(static_cast<double>(bandwidth + processingPower));
        websiteDummyWeights2.push_back(static_cast<double>(bandwidth + processingPower));
        website_round.push_back(x);
    }

    void initialweightBalancer() {
        double sumOfWeights = 0;
        for (int i = 0; i < websiteWeights.size(); i++) {
            if (requestQueues[i].empty() != 1) {
                sumOfWeights = sumOfWeights + websiteWeights[i];
            }
        }
        for (int i = 0; i < websiteWeights.size(); i++) {
            websiteWeights[i] = static_cast<double>((websiteWeights[i] / sumOfWeights) * TOTAL_BANDWIDTH);
            websiteDummyWeights[i] = websiteWeights[i];
            websiteDummyWeights2[i] = websiteWeights[i];
        }
    }

    void weightBalancer() {
        double sumOfWeights = 0;
        for (int i = 0; i < websiteWeights.size(); i++) {
            if (requestQueues[i].empty() != 1) {
                sumOfWeights = sumOfWeights + websiteWeights[i];
            }
        } // total sum of weights of non-empty queues
        for (int i = 0; i < websiteWeights.size(); i++) {
            websiteWeights[i] = static_cast<double>((websiteWeights[i] / sumOfWeights) * TOTAL_BANDWIDTH);
            websiteDummyWeights[i] = websiteDummyWeights[i] + websiteWeights[i] - websiteDummyWeights2[i];
            websiteDummyWeights2[i] = websiteWeights[i];
        } // adjusted weights
    }

    void enqueueRequest(HttpRequest request) {
        // Enqueue an HTTP request into the corresponding website's queue
        int websiteIndex = findWebsiteIndex(request.getWebsiteID());
        if (websiteIndex != -1) {
            requestQueues[websiteIndex].push(request);
        }
    }

    HttpRequest dequeueRequest() {
        // Dequeue the next HTTP request based on the WFQ scheduling algorithm
        int nextWebsiteIndex = getNextWebsiteIndex();
        int nextWebsiteWeight = getNextWebsiteWeight(nextWebsiteIndex);
        int dummyWeight = getNextWebsiteDummyWeight(nextWebsiteIndex);
        sum1 = nextWebsiteIndex;
        sum2 = nextWebsiteWeight;
        sum3 = dummyWeight;
        if (nextWebsiteIndex != -1 && !requestQueues[nextWebsiteIndex].empty()) {
            HttpRequest nextRequest = requestQueues[nextWebsiteIndex].front();
            requestQueues[nextWebsiteIndex].pop();
            return nextRequest;
        }
        // Return a default HttpRequest if no requests are in the queue
        return HttpRequest(-1, -1, -1);
    }

    int findWebsiteIndex(int websiteID) {
        for (int i = 0; i < websites.size(); i++) {
            if (websites[i].getWebsiteID() == websiteID) {
                return i;
            }
        }
        return -1;
    }

    int getNextWebsiteIndex() {
        // Implement the WFQ scheduling algorithm here
        // get minimum round
        int min_round = 100;
        for (int i = 0; i < requestQueues.size(); i++) {
            if (website_round[i] <= min_round) {
                min_round = website_round[i];
            }
        }
        int selectedWebsiteIndex = -1;
        double maxWeight = 0.0;
        for (int i = 0; i < requestQueues.size(); i++) {
            if (!requestQueues[i].empty() && website_round[i] == min_round) { // get maximum weight from the ones having minimum round
                double weight = websiteWeights[i];
                if (weight > maxWeight) {
                    maxWeight = weight;
                    selectedWebsiteIndex = i;
                }
            }
        } // select website index with maximum weight
        if (selectedWebsiteIndex == -1) { // Yet another edge case
            while (selectedWebsiteIndex == -1) {
                min_round++;
                for (int i = 0; i < requestQueues.size(); i++) {
                    if (!requestQueues[i].empty() && website_round[i] == min_round) { // get maximum weight from the ones having minimum round
                        double weight = websiteWeights[i];
                        if (weight > maxWeight) {
                            maxWeight = weight;
                            selectedWebsiteIndex = i;
                        }
                    }
                }
            }
        }
        websiteDummyWeights[selectedWebsiteIndex]--;
        int selectedWebsiteIndexPrev = selectedWebsiteIndex;
        while (websiteDummyWeights[selectedWebsiteIndex] < 0 || requestQueues[selectedWebsiteIndex].empty() == 1) {
            website_round[selectedWebsiteIndexPrev]++; // round=0->round=1;
            count_while++;
            if (requestQueues[selectedWebsiteIndex].empty() != 1) {
                websiteDummyWeights[selectedWebsiteIndex] = websiteWeights[selectedWebsiteIndex];
            }
            // get minimum round again (edge case) (repeating 1's (last website of a round))
            int min_round = 100;
            for (int i = 0; i < requestQueues.size(); i++) {
                if (requestQueues[i].empty() != 1) {
                    if (website_round[i] <= min_round) {
                        min_round = website_round[i];
                    }
                }
            }
            maxWeight = 0.0;
            for (int i = 0; i < requestQueues.size(); i++) {
                if (i != selectedWebsiteIndexPrev && website_round[i] == min_round) { // select maximum weight from other remaining ones
                    count++;
                    if (!requestQueues[i].empty()) {
                        double weight = websiteWeights[i];
                        if (weight > maxWeight) {
                            maxWeight = weight;
                            selectedWebsiteIndex = i;
                        }
                    }
                }
            }
            websiteDummyWeights[selectedWebsiteIndex]--;
        }
        return selectedWebsiteIndex;
    }

    int getNextWebsiteWeight(int index) {
        return websiteWeights[index];
    }

    int getNextWebsiteDummyWeight(int index) {
        return websiteDummyWeights[index];
    }
};

int main() {
    LoadBalancer loadBalancer;
    vector<HttpRequest> requestsStore;
    int num_requests = 0;
    int num_websites=0;


	
	cout <<"\nEnter the number of Websites: ";
	cin>>num_websites;
	cout<<"\nEnter the Bandwidth (number of Requests served in one round, such that Website with the minimum weight has atleast one request per cycle): ";
	cin>>TOTAL_BANDWIDTH;
    cout << "\nEnter the number of Requests: ";
    cin >> num_requests;
    cout << "\n\n";

    if (num_requests > 0 && num_websites > 0) {
        // Add websites with their attributes
        
        
      /*  loadBalancer.addWebsite(1, 101, 800, 900);
        loadBalancer.addWebsite(2, 102, 1000, 1000);
        loadBalancer.addWebsite(3, 103, 1500, 1000);
        loadBalancer.addWebsite(4, 104, 1000, 2000);*/
        
        for(int i=1;i<=num_websites;i++){
        	loadBalancer.addWebsite(i,100+i,100*i,100*i);
        }

        for (int i = 1; i <= num_requests; i++) {
            HttpRequest request(i, (i % num_websites) + 1, 100); // Example HttpRequest creation
            loadBalancer.enqueueRequest(request);
        }

        loadBalancer.initialweightBalancer();
        
        if(loadBalancer.websiteWeights[0]<1){
        	cout<<"\nEnter a valid Bandwidth!!\n ";
        	
        	exit(0);
        }

        int i = 0;
        int totalTime = 0;

        while (1) {
        
        	
        
        	std::this_thread::sleep_for(std::chrono::milliseconds(50));
        	
            HttpRequest nextRequest = loadBalancer.dequeueRequest();
            requestsStore.push_back(nextRequest);
            totalTime += requestsStore[requestsStore.size() - 1].getProcessingTime();
            cout << "\n";

            cout << "Next Website Index: " << loadBalancer.sum1 << "\n";
            cout << "Rebalanced Weight: " << loadBalancer.sum2 << "\n";
            loadBalancer.weightBalancer();

            int color = 90 + nextRequest.getWebsiteID();
            cout << "\033[" << color << "m";
            cout << "Processing Request " << nextRequest.getRequestID() << " for Website " << nextRequest.getWebsiteID();
            cout << "\033[0m\n" ;

            if (loadBalancer.requestQueues[nextRequest.getWebsiteID() - 1].empty()) {
                cout << "\033[" << color-50 << "m";
                cout << "\n------------------------------------------------------";
                cout << "\nAll requests for " << nextRequest.getWebsiteID() << " are processed!! " << endl;
                cout << "------------------------------------------------------";
                cout << "\nTimestamp: " << totalTime << endl;
                cout << "------------------------------------------------------\033[0m" << endl;
            }

            i++;
            if (i == num_requests) {
                break;
            }
        }
    } else {
        cout << "No requests/websites present! Enter valid data! " << endl;
    }

    return 0;
}





