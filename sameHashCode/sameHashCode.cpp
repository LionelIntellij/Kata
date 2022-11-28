#include <thread>
#include <iostream>
#include <random>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm>
#include <mutex>
#include <math.h>
#include <future>
#include <set>
#include <array>
#include  <map>
#include <string>
#include <sstream>
#include <functional>

#define NB_ALPHA  62
#define SIZE_HASH 5
#define TIME_INTERVAL_BETWEEN_SHOW 10


static const char alphabeticsLetters[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";

std::string getGenerateString(const long long id)
{
    /**\brief generate string from id : consider that each element of possible string correspond to a number
    * \return : the generated string 
    **/

    std::string generatedStr;
    generatedStr.resize(SIZE_HASH, alphabeticsLetters[0]);

    long long limit = NB_ALPHA;
    long long iteration = 0;
    while (limit <= id)
    {
        limit *= NB_ALPHA;
        iteration++;
    }

    if (iteration > 0)
        limit /= NB_ALPHA;

    long long r = id;
    long long lastId = r;
    long long idAlpha;
    while (r >= NB_ALPHA )
    {
        idAlpha = lastId / limit;
        r = lastId % limit;
        generatedStr[iteration] = alphabeticsLetters[idAlpha];
        limit /= NB_ALPHA;
        lastId = r;
        iteration--;
    }
    generatedStr[0] = alphabeticsLetters[r];
    
    return generatedStr;
}

std::string getGenerateRandomString() {
    auto randchar = []() -> char
    {
        const char charset[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890";
        const size_t max_index = (sizeof(charset) - 1);
        return charset[rand() % max_index];
    };
    std::string str(4, 0);
    std::generate_n(str.begin(), SIZE_HASH, randchar);
    return str;
}

void getHashPartialCodeCollision(std::string& str1, std::string& str2, std::string str3, long long idStr1
   , std::future<void>& futureExitObj)
{
    long long nbrCombination = static_cast<long long>(std::pow(NB_ALPHA, SIZE_HASH));
   
    std::hash<std::string> strHash;
    
    int nbCollision = 0;
    std::string tmpStr;
    long long TimeToShow = TIME_INTERVAL_BETWEEN_SHOW;
    for (long long j = idStr1 + 1; j < nbrCombination; ++j)
    {
        str1 = getGenerateRandomString();
        size_t hash1 = strHash(str1);
        tmpStr = getGenerateRandomString();
        size_t hash2 = strHash(str2);
        if (hash2 == hash1)
        {
            nbCollision++;
            if (nbCollision == 1)
            {
                str2 = tmpStr;
                std::string msg = " str1 " + str1 + " str2 " + str2 + " hash" + std::to_string(hash1);
                std::cout << msg << std::endl;
                break;
            }
            //if (nbCollision > 1)
            //{
            //    str3 = tmpStr;
            //    break;
            //}
        }
    }
}


struct ResultApp
{
    /**\brief result obtained when a collision is found
    * \element  str1: first string element collision
    *           str2: first string element collision
    *           idFirstEltStr id of the first element of generated string
    *           idLastEltStr  id of the last element of generated string
    *           stateExitSignal : signal allowing to inform if the thread finished the task
    **/
    ResultApp():stateExitSignal(nullptr),
                futureExitObj(nullptr),
                idStr1 (0){}
    ~ResultApp()
    { 
        delete stateExitSignal;
        delete futureExitObj;
    }
    std::string str1;
    std::string str2;
    std::string str3;
    long long idStr1;
    std::future<void>* futureExitObj;
    std::promise<bool>* stateExitSignal;
};

struct ThreadApp
{
    /**\brief result obtained when a collision is found
    * \element  thread: thread used to run the collision detection.
    *           futureStateExitObj: object allowing to inform if the thread finish the task.
    **/
    ThreadApp() :thread(nullptr),
        futureStateExitObj(nullptr),
        exitSignal(nullptr){}
    ~ThreadApp() 
    {
        delete thread;
        delete futureStateExitObj;
        delete exitSignal;
    }
    std::thread* thread;
    std::promise<void>* exitSignal;
    std::future<bool>* futureStateExitObj;
};



void instanciateThread(ResultApp& resultApp, ThreadApp & threadApp)
{
    /**\brief instanciate the different thread 
    * \element  resultApp: Data struct containing all variables about the collision detection result 
    *           threadApp: Data struct containing all variables necessary to the thread management.
    **/
    if (threadApp.futureStateExitObj)
        delete threadApp.futureStateExitObj;
    if (resultApp.stateExitSignal)
        delete resultApp.stateExitSignal;
    if (threadApp.exitSignal)
        delete threadApp.exitSignal;
    if (resultApp.futureExitObj)
        delete resultApp.futureExitObj;
    if (threadApp.thread)
        delete threadApp.thread;

   threadApp.futureStateExitObj = new std::future<bool>();
   resultApp.stateExitSignal = new std::promise<bool>();
   *threadApp.futureStateExitObj = resultApp.stateExitSignal->get_future();

   resultApp.futureExitObj = new std::future<void>();
   threadApp.exitSignal = new std::promise<void>();
   *resultApp.futureExitObj = threadApp.exitSignal->get_future();

    threadApp.thread = new std::thread([&resultApp]()
        {
            resultApp.str1.clear();
            resultApp.str2.clear();
            getHashPartialCodeCollision(resultApp.str1, resultApp.str2, resultApp.str3,
                resultApp.idStr1, *resultApp.futureExitObj);
            resultApp.stateExitSignal->set_value(true);
        });
}


int main(int argc, char** argv)
{

    //get total number of logical core on the computer
    const auto processorCount = std::thread::hardware_concurrency();
    unsigned int processorCountAvailable = 1;
    if (processorCount != 0)
    {
        std::cout << "The application will be  multithread on " << processorCount << " threads" << std::endl;
        processorCountAvailable = processorCount - 1;//one core dedicated to the main thread
    }
    
    std::vector<ThreadApp> threadsApp;
    std::vector<ResultApp> resultsApp;
    resultsApp.resize(processorCountAvailable);
    threadsApp.resize(processorCountAvailable);

    //Compute the number of different string possibilities with SIZE_HASH characters
    int nbrCombination = static_cast<int>(std::pow(NB_ALPHA, SIZE_HASH));
    int nbrCombinationPerThread = nbrCombination / processorCountAvailable;

    //Initialize clock    
    auto start = std::chrono::system_clock::now();
    //Instantiate the threads according to the number of available logical cores.
    int idFirstEltStrNextThread = 0;
    for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
    {
        ResultApp& result = resultsApp[proc];
        result.idStr1 = proc; //initiatialise first combinaison element to search collision 
        std::cout << "Starting thread " + std::to_string(proc + 1) <<std::endl;;
        instanciateThread(resultsApp[proc], threadsApp[proc]);
    }

    long long idMaxElt = processorCountAvailable;
    bool allThreadTerminatedTask = false;
    //thread management
    bool isCollisionFound = false;
    std::array<std::string, 3> collisions;
   
    while (!isCollisionFound || idMaxElt < nbrCombination && !allThreadTerminatedTask)
    {
        allThreadTerminatedTask = true;
        for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        {
            ThreadApp& thread = threadsApp[proc];
            if (!thread.thread->joinable())
                continue;
            allThreadTerminatedTask = false;

            if (thread.futureStateExitObj->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) //check if this thread has finished her task
            {
                ResultApp& result = resultsApp[proc];
                thread.thread->join(); //stop thread
                if (!result.str3.empty()) // if all the necessary collision is found st 
                {
                    collisions =  { result.str1, result.str2,result.str3 };
                    isCollisionFound = true;
                }
                else // collision has been found relaunch the thread with new hash value to find
                {
                    result.idStr1 = idMaxElt;
                    instanciateThread(resultsApp[proc], threadsApp[proc]);
                    idMaxElt++;
                    if(idMaxElt% TIME_INTERVAL_BETWEEN_SHOW == 0)
                        std::cout<<" State " + std::to_string(idMaxElt) +" on "+ std::to_string(nbrCombination) + " %" << std::endl;
                }
            }
        }
    }
   
    //Release all threads. All necessary collisions are found. 
    for (ThreadApp& thread : threadsApp)
    {
        thread.exitSignal->set_value();
        if (thread.thread->joinable())
            thread.thread->join();
    }

    //Stop clock and show result
    std::chrono::duration<double> diff = std::chrono::system_clock::now() - start;
    std::hash<std::string> strHash;
    if (isCollisionFound)
    {
        std::cout << "The 3 same collisions founded are : " << std::endl;
        std::cout << "str1 : " << collisions.at(0) << " str2 : " << collisions.at(1) << " str3 " << collisions.at(2) << " Hash code " << strHash(collisions.at(0)) << std::endl;

        std::cout << "The collision detection took  : " << diff.count() << " seconds " << std::endl;
    }
    else
    {
        std::cout << "No collisions found " << std::endl;
    }
    return 0;
}
