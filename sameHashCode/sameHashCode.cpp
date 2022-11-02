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

#define NB_ALPHA  26
#define SIZE_HASH 3
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
    int lastId = id;
    while (r >= NB_ALPHA)
    {
        int idAlpha = lastId / limit;
        r = lastId % limit;
        generatedStr[iteration] = alphabeticsLetters[idAlpha];
        limit /= NB_ALPHA;
        lastId = r;
        iteration--;
    }
    generatedStr[iteration] = alphabeticsLetters[r];
    
    return generatedStr;
}


int getIdFromString(const std::string& generatedStr)
{
    /**\brief determinate id from the generated string
    * \arg generatedStr: The generated string 
    * \return  id of the generated string
    **/

    int iteration = SIZE_HASH - 1;
    int limit = 1;
    int factor = 1;
    int firstIdEltNoNull = iteration;
    while (iteration > 0)
    {
        if (lettersId[generatedStr[iteration]] != 0 && factor != NB_ALPHA)
        {
            factor = NB_ALPHA;
            firstIdEltNoNull = iteration;
        }
        limit *= factor;
        iteration--;
    }
    iteration = firstIdEltNoNull;
    int id = 0;
    while (iteration >= 0)
    {
        id += lettersId[generatedStr[iteration]] * limit;
        limit /= NB_ALPHA;
        iteration--;
    }
    return id;
}

void getHashCodeCollisionOptim(std::string& str1, std::string& str2,
    int firstId, int lastId, int initJ,
    std::future<void>& futureExitObj,
    int threadID)
{
    /**\brief detection of the collison of hash function
    * \arg str1: first string element collision
    *      str2: first string element collision
    *      futureExitObj: object allowing to stop the thread
    * \return  generated string
    **/
    std::hash<std::string> strHash;
    int k = initJ;
    //Compute the total comparaison number to do
    int  n = lastId - firstId;
    int totalComparaison = static_cast<int>(n * (n - 1) * 0.5);
    std::cout << "Thread " <<threadID<<" Starting collision detection search with " << totalComparaison <<" comparisons" <<std::endl;
    int nbComparisonDone = 0;
    for (int i = firstId; i < lastId - 1; i++)
    {
        str1 = getGenerateString(i);
        for (int j = k; j < lastId; j++)
        {
            str2 = getGenerateString(j);
            if (strHash(str1) == strHash(str2) && str1 != str2) //You find a collision
                return;
            if (futureExitObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready) //Other threads find  all necessary collisions
            { // no collision found clear all result
                str1.clear();
                str2.clear();
                return;
            }
        }

        //compute the percentage of comparison already done
        nbComparisonDone += lastId - k;
        int pourcentDone = static_cast<int>((nbComparisonDone / static_cast<double>(totalComparaison)) * 100.);
        k = i + 1;
        std::string msg = " Thread  " + std::to_string(threadID) + " : Task Total comparison " + std::to_string(totalComparaison) + " Pourcent comparison done " + std::to_string(pourcentDone) + " %\n";
        std::cout << msg;
    }
    // no collision found clear all result
    str1.clear();
    str2.clear();
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
                idInitCmpELt(0),
                threadID(0){}
    ~ResultApp()
    { 
        delete stateExitSignal;
    }
    std::string str1;
    std::string str2;
    int idFirstEltStr;
    int idInitCmpELt;
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

    std::cout << "Starting thread " << resultApp.threadID << std::endl;
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
            getHashCodeCollisionOptim(resultApp.str1, resultApp.str2,
                resultApp.idFirstEltStr, resultApp.idLastEltStr, resultApp.idInitCmpELt,futureObj, resultApp.threadID);
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
        result.idInitCmpELt = result.idFirstEltStr + 1;
        idFirstEltStrNextThread = result.idLastEltStr;
        instanciateThread(resultsApp[proc], threadsApp[proc], futureExitObj);
    }

    //thread management
    std::map<size_t,std::pair<std::string,std::string>> collisionFound;
    std::hash<std::string> strHash;
    std::vector<std::array<std::string, 3>> collisionsThreeElt;
    bool allThreadTerminatedTask = false;
    collisionsThreeElt.reserve(processorCountAvailable);
    while (collisionsThreeElt.empty() && !allThreadTerminatedTask)
    {
        allThreadTerminatedTask = true;
        for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        {
            ThreadApp& thread = threadsApp[proc];
            if (!thread.thread->joinable())
                continue;

            allThreadTerminatedTask = false; // a thread is not terminated
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

                //if collision has been found and the task of the thread is not finished 
                //launch the thread on the rest of the combinations to test
                int i = getIdFromString(result.str1);
                int j = getIdFromString(result.str2);

                if (i != result.idLastEltStr - 2
                    || j != result.idLastEltStr - 1)
                {
                    std::cout << "reStarting thread " << result.threadID << std::endl;
                    result.idFirstEltStr = i;
                    result.idInitCmpELt = j + 1;
                    instanciateThread(result, thread, futureExitObj);
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
