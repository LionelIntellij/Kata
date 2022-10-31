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
#include <array>

#define NB_ALPHA  26
#define SIZE_HASH 4 

static char alphabeticsLetters[NB_ALPHA] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                             'p','q','r','s','t','u','v','w','x','y','z' };


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
        limit = limit * NB_ALPHA;
        iteration++;
    }

    if (iteration > 0)
        limit /= NB_ALPHA;

    iteration = 0;
    int r = id;
    int lastId = id;
    while (r >= NB_ALPHA)
    {
        int idAlpha = lastId / limit;
        r = lastId % limit;
        generatedStr[iteration] = alphabeticsLetters[idAlpha];
        limit /= NB_ALPHA;
        lastId = r;
        iteration++;
    }
    generatedStr[iteration] = alphabeticsLetters[r];
    
    return generatedStr;
}


void getHashCodeCollisionOptim(std::string& str1, std::string& str2, 
                                int firstId, int lastId,
                                std::future<void>& futureExitObj)
{
    /**\brief detection of the collison of hash function
  * \arg str1: first string element collision
  *      str2: first string element collision
  *      futureExitObj: object allowing to stop the thread
  * \return  generated string
  **/
    std::hash<std::string> strHash;
    for (int i = firstId; i < lastId - 1 ; i++)
    {
        str1 = getGenerateString(i);
        for (int j = i +1; j< lastId; j++)
        {
            str2 = getGenerateString(j);
            if (strHash(str1) == strHash(str2) //You find a collision
                || futureExitObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout) //Other threads find  all necessary collisions
            {
                return;
            }
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
                idFirstEltStr(0),
                idLastEltStr(0){}
    ~ResultApp()
    { 
        delete stateExitSignal;
    }
    std::string str1;
    std::string str2;
    int idFirstEltStr;
    int idLastEltStr;
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

   threadApp.futureStateExitObj = new std::future<bool>();
   resultApp.stateExitSignal = new std::promise<bool>();
   *threadApp.futureStateExitObj = resultApp.stateExitSignal->get_future();

    threadApp.thread = new std::thread([&resultApp, &futureObj]()
        {
            resultApp.str1.clear();
            resultApp.str2.clear();
            getHashCodeCollisionOptim(resultApp.str1, resultApp.str2, 
                resultApp.idFirstEltStr, resultApp.idLastEltStr, futureObj);
            resultApp.stateExitSignal->set_value(true);
        });
}

int main(int argc, char** argv)
{

    //get total number of logical core on the computer
    const auto processorCount = std::thread::hardware_concurrency();
    if (processorCount == 0)
        std::cout << " INFO: The application will be not multithreaded" << std::endl;

    //core dedicated to the main thread
    auto processorCountAvailable = processorCount - 1;
    if (processorCountAvailable < 1)
        processorCountAvailable = 1;

    std::vector<ThreadApp> threadsApp;
    std::vector<ResultApp> resultsApp;
    resultsApp.resize(processorCountAvailable);
    threadsApp.resize(processorCountAvailable);

    //Initialize clock    
    auto start = std::chrono::system_clock::now();

    //Signal to exit all the active thread
    std::promise<void> exitSignal; 
    std::future<void> futureExitObj = exitSignal.get_future();

    //Compute the number of different string possibilities with SIZE_HASH characters
    int nbrCombination = static_cast<int>(std::pow(NB_ALPHA, SIZE_HASH));
    int nbrCombinationPerThread = nbrCombination / processorCountAvailable;

    //Instantiate the threads according to the number of available logical cores.
    int idFirstEltStrNextThread = 0;
    for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
    {
        ResultApp& result = resultsApp[proc];
        result.idFirstEltStr = idFirstEltStrNextThread;
        if (proc + 1 != processorCountAvailable)
            result.idLastEltStr = (proc + 1) * nbrCombinationPerThread;
        else
            result.idLastEltStr = nbrCombination;
        idFirstEltStrNextThread = result.idLastEltStr;
        instanciateThread(resultsApp[proc], threadsApp[proc], futureExitObj);
    }

    //thread management
    std::vector<std::array<std::string,2>> collisionFound;
    while (collisionFound.size() < 3)
    {
        for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        {
            ThreadApp& thread = threadsApp[proc];
            auto duration = std::chrono::milliseconds(0);
            auto status = thread.futureStateExitObj->wait_for(duration);
            if (status == std::future_status::ready && thread.thread->joinable())
            {
                ResultApp& result = resultsApp[proc];
                std::array<std::string, 2> array = { result.str1, result.str2 };
                 collisionFound.push_back(array);
                 thread.thread->join();  
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
    std::hash<std::string> strHash;
    for (const auto& collision : collisionFound)
        std::cout <<"str1 : "<<collision.at(0) << " str2 : " << collision.at(1) << " Hash code " << strHash(collision.at(0)) << std::endl;
    std::cout << std::endl;
    std::cout << "The collision detection took  : " <<diff.count()<<" seconds "<<std::endl;
}


