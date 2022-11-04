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

#define NB_ALPHA  26
#define SIZE_HASH 4
#define TIME_INTERVAL_BETWEEN_SHOW 1

static char alphabeticsLetters[NB_ALPHA] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                             'p','q','r','s','t','u','v','w','x','y','z' };
static std::map<char, int> lettersId;


std::string getGenerateString(const int id)
{
    /**\brief generate string from id : consider that each element of possible string correspond to a number
    * \return : the generated string 
    **/

    std::string generatedStr;
    generatedStr.resize(SIZE_HASH, alphabeticsLetters[0]);

    int limit = NB_ALPHA;
    int iteration = 0;
    while (limit <= id)
    {
        limit *= NB_ALPHA;
        iteration++;
    }

    if (iteration > 0)
        limit /= NB_ALPHA;

    int r = id;
    int lastId = r;
    while (r >= NB_ALPHA )
    {
        int idAlpha = lastId / limit;
        r = lastId % limit;
        generatedStr[iteration] = alphabeticsLetters[idAlpha];
        limit /= NB_ALPHA;
        lastId = r;
        iteration--;
    }
    generatedStr[0] = alphabeticsLetters[r];
    
    return generatedStr;
}


int getIdFromString(const std::string& generatedStr)
{
    /**\brief determinate id from the generated string
    * \arg generatedStr: The generated string 
    * \return  id of the generated string
    **/

    int id = lettersId[generatedStr[0]];
    int factor = NB_ALPHA;
    for(int i = 1; i<  SIZE_HASH - 1; i++)
    {
        id += lettersId[generatedStr[i]] *factor;
        factor *= NB_ALPHA;
    }
    return id;
}


void getHashCodeCollision(std::string& str1, std::string& str2,
    int firstId, int lastId, std::future<void>& futureExitObj,
    int threadID)
{
    /**\brief detection of the collison of hash function
    * \arg str1: first string element collision
    *      str2: first string element collision
    *      futureExitObj: object allowing to stop the thread
    *      threadID: the thread id . It's only necessary to show message
    * \return  generated string
    **/
    
    std::map<size_t, int> buf;
    int size =lastId - firstId;
    std::string msg = "Thread " + std::to_string(threadID) + " Starting collision detection search with " + std::to_string(size) + " to test \n";
    std::cout << msg;
    std::hash<std::string> strHash;
    for (int i = firstId; i < lastId; ++i)
    {
        str1 = getGenerateString(i);
        size_t hash = strHash(str1);
        auto it = buf.find(hash);
        if ( it== buf.end())
            buf.insert(std::pair<size_t, int>(hash, i));
        else
        {
            str2 = getGenerateString(it->second);
            break;
        } 
        if (futureExitObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) //Other threads find  all necessary collisions
            break;
    }
    str1.clear();
      
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
                idFirstEltStr(0),
                idLastEltStr(0),
                threadID(0){}
    ~ResultApp()
    { 
        delete stateExitSignal;
    }
    std::string str1;
    std::string str2;
    int idFirstEltStr;
    int idLastEltStr;
    int threadID;
    std::promise<bool>* stateExitSignal;
};

struct ThreadApp
{
    /**\brief result obtained when a collision is found
    * \element  thread: thread used to run the collision detection.
    *           futureStateExitObj: object allowing to inform if the thread finish the task.
    **/
    ThreadApp() :thread(nullptr),
        futureStateExitObj(nullptr){}
    ~ThreadApp() 
    {
        delete thread;
        delete futureStateExitObj;
    }
    std::thread* thread;
    std::future<bool>* futureStateExitObj;
};



void instanciateThread(ResultApp& resultApp, ThreadApp & threadApp, std::future<void> & futureObj)
{
    /**\brief instanciate the different thread 
    * \element  resultApp: Data struct containing all variables about the collision detection result 
    *           threadApp: Data struct containing all variables necessary to the thread management.
    **/
    std::string msg = "Starting thread " + std::to_string(resultApp.threadID)+"\n";
    std::cout << msg;
    if (threadApp.futureStateExitObj)
        delete threadApp.futureStateExitObj;
    if (resultApp.stateExitSignal)
        delete resultApp.stateExitSignal;
    if (threadApp.thread)
        delete threadApp.thread;

   threadApp.futureStateExitObj = new std::future<bool>();
   resultApp.stateExitSignal = new std::promise<bool>();
   *threadApp.futureStateExitObj = resultApp.stateExitSignal->get_future();
    threadApp.thread = new std::thread([&resultApp, &futureObj]()
        {
            resultApp.str1.clear();
            resultApp.str2.clear();
            getHashCodeCollision(resultApp.str1, resultApp.str2,
                resultApp.idFirstEltStr, resultApp.idLastEltStr,futureObj, resultApp.threadID);
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

    //get the id of the letter
    for (int i = 0; i< NB_ALPHA; ++i)
        lettersId.insert(std::pair<char, int>(alphabeticsLetters[i], i));

    //Signal to exit all the active thread
    std::promise<void> exitSignal; 
    std::future<void> futureExitObj = exitSignal.get_future();

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
        result.threadID = proc + 1;
        result.idFirstEltStr = idFirstEltStrNextThread;
        if (proc + 1 != processorCountAvailable)
            result.idLastEltStr = (proc + 1) * nbrCombinationPerThread;
        else
            result.idLastEltStr = nbrCombination;
        idFirstEltStrNextThread = result.idLastEltStr;
        instanciateThread(resultsApp[proc], threadsApp[proc], futureExitObj);
    }

    //thread management
    std::map<size_t,std::pair<std::string,std::string>> collisionFound;
    std::hash<std::string> strHash;
    std::vector<std::array<std::string, 3>> collisionsThreeElt;
    collisionsThreeElt.reserve(processorCountAvailable);
    bool allThreadTerminatedTask = false;
    while (collisionsThreeElt.empty()  && !allThreadTerminatedTask)
    {
        allThreadTerminatedTask = true;
        for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        {
            ThreadApp& thread = threadsApp[proc];
            if (!thread.thread->joinable())
                continue;
            allThreadTerminatedTask = false;
            if (thread.futureStateExitObj->wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) //check if this thread has finished has task
            {
                ResultApp& result = resultsApp[proc];
                thread.thread->join(); //stop thread
                std::cout << "Stop thread " << result.threadID << std::endl;
                if (result.str1.empty()) // if not collision has been found and the thread task is finished continue to next thread.
                    continue;

                std::cout << " Collision found by the thread " << result.threadID << std::endl;
                size_t hashCode = strHash(result.str1);
                auto it = collisionFound.find(hashCode);
                if (it == collisionFound.end()) //If new collision found keep it in map with hash code as key
                {
                    std::pair<std::string, std::string> collision(result.str1, result.str2);
                    collisionFound.insert(std::pair<size_t, std::pair<std::string, std::string>>(hashCode, collision));
                }
                else //if collision found and already exist, check if the string are the same
                {
                    if (result.str1 != it->second.second
                        && result.str1 != it->second.first)
                    {
                        std::array<std::string, 3> collisionThreeElt = { result.str1,it->second.second ,it->second.first };
                        collisionsThreeElt.push_back(collisionThreeElt);
                    }
                    
                    if (result.str2 != it->second.second
                        && result.str2 != it->second.first)
                    {
                        std::array<std::string, 3> collisionThreeElt = { result.str2,it->second.second ,it->second.first };
                        collisionsThreeElt.push_back(collisionThreeElt);
                    }
                }
            }
        }
    }

    //Release all threads. All necessary collisions are found. 
    exitSignal.set_value(); //Set the value
    for (ThreadApp& thread : threadsApp)
        if(thread.thread->joinable())
            thread.thread->join();

    //Stop clock and show result
    std::chrono::duration<double> diff = std::chrono::system_clock::now() - start;
    std::cout << "The collisions founded are : " << std::endl;
    for (const auto& collision : collisionFound)
        std::cout <<"str1 : "<<collision.second.first << " str2 : " << collision.second.second<< " Hash code " << collision.first << std::endl;
    std::cout << std::endl;

    std::cout << "The 3 same collisions founded are : " << std::endl;
    for (auto arr : collisionsThreeElt)
        std::cout << "str1 : " << arr.at(0) << " str2 : " << arr.at(1) << " str3 " << arr.at(2) << " Hash code " << strHash(arr.at(0)) << std::endl;

    std::cout << "The collision detection took  : " <<diff.count()<<" seconds "<<std::endl;
}
