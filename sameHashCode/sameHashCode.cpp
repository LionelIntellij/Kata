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

#define CH_MAX 26
#define SIZE_HASH 4

static char alphabeticsLetters[CH_MAX] = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o',
                             'p','q','r','s','t','u','v','w','x','y','z' };



std::string randomString(const size_t length)
{
   /**\brief generate random strings
    * \arg lenght: size of generated string  
    * 
    * \return  generated string 
    **/
    std::string str = "";
    str.reserve(length);
    for (int i = 0; i < length; i++)
        str += alphabeticsLetters[rand() % CH_MAX];
    return str;
}

void getHashCodeCollision(std::string& str1, std::string& str2, size_t lenght, std::future<void>& futureExitObj)
{
  /**\brief detection of the collison of hash function
    * \arg str1: first string element collision 
    *      str2: first string element collision 
    *      futureExitObj: slot allowing to stop the thread
    * \return  generated string
    **/
    str1 = randomString(lenght);
    str2 = randomString(lenght);
    std::hash<std::string> strHash;
    int it = 0;
    while (futureExitObj.wait_for(std::chrono::milliseconds(1)) == std::future_status::timeout &&
        strHash(str1) != strHash(str2) || str1 == str2 && it <100)
    {
        str1 = randomString(lenght);
        str2 = randomString(lenght);
        it++;
    }
}

struct ResultApp
{
    /**\brief result obtained when a collision is found
    * \element  str1: first string element collision
    *           str2: first string element collision
    *           stateSignal: signal allowing to inform the state of the thread
    **/
    ResultApp():stateSignal(nullptr){}
    ~ResultApp()
    { 
        delete stateSignal; 
    }
    std::string str1;
    std::string str2;
    std::promise<bool>* stateSignal; // Create a std::promise object
};

struct ThreadApp
{
    /**\brief result obtained when a collision is found
    * \element  thread: thread used to run the collision detection
    *           futureStateObj: slot allowing to get the state of the thread.
    **/
    ThreadApp() :thread(nullptr),
        futureStateObj(nullptr){}
    ~ThreadApp() 
    {
        delete thread;
        delete futureStateObj;
    }
    std::thread* thread;
    std::future<bool>* futureStateObj;
};



void instanciateThread(ResultApp& resultApp, ThreadApp & threadApp, std::future<void> & futureObj)
{
   threadApp.futureStateObj = new std::future<bool>();
   resultApp.stateSignal = new std::promise<bool>();
   *threadApp.futureStateObj = resultApp.stateSignal->get_future();
    threadApp.thread = new std::thread([&resultApp, &futureObj]()
        {
            resultApp.str1.clear();
            resultApp.str2.clear();
            getHashCodeCollision(resultApp.str1, resultApp.str2, SIZE_HASH, futureObj);
            resultApp.stateSignal->set_value(true);
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

    //initialize clock    
    auto start = std::chrono::system_clock::now();

    // signal to exit all the active thread
    std::promise<void> exitSignal; 
    std::future<void> futureExitObj = exitSignal.get_future();

    //instanciate thread according the number of available logical core
    for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        instanciateThread(resultsApp[proc], threadsApp[proc], futureExitObj);

    //Manage thread
    std::vector<std::string> collisionFound;
    while (collisionFound.size() != 3)
    {
        for (unsigned int proc = 0; proc < processorCountAvailable; ++proc)
        {
            ThreadApp& thread = threadsApp[proc];
            ResultApp& result = resultsApp[proc];
            auto duration = std::chrono::milliseconds(0);
            auto status = thread.futureStateObj->wait_for(duration);
            if (status == std::future_status::ready)
            {
                if (collisionFound.empty())
                {
                    collisionFound.push_back(result.str1);
                    collisionFound.push_back(result.str2);
                    thread.thread->join();
                    //TODO instanciate a new thread.
                }
                else
                {
                    thread.thread->join();
                    auto itStr1 = std::find(collisionFound.begin(), collisionFound.end(), result.str1);
                    if (itStr1 == collisionFound.end())
                    {
                        auto itStr2 = std::find(collisionFound.begin(), collisionFound.end(), result.str2);
                        if (itStr2 != collisionFound.end())
                            collisionFound.push_back(*itStr2);
                        //else//TODO instanciate a new thread.
                        // instanciateThread(result, futureObj);
                            
                    }
                    else
                        collisionFound.push_back(*itStr1);
                }
            }
        }
    }

    //free all threads. All collisions are found. 
    std::cout << "Asking the thread to stop" << std::endl;
    exitSignal.set_value(); //Set the value
    for (ThreadApp& thread : threadsApp)
        thread.thread->join();

    //Stop clock
    std::chrono::duration<double> diff = std::chrono::system_clock::now() - start;
    std::cout << "The collision founded are : " << std::endl;
    for (const std::string& collision : collisionFound)
        std::cout << collision << std::endl;
    std::cout << "The detection took  : " <<diff.count()<<" s"<<std::endl;
}




























/*
for ()
{
    for ()
    {

    }
}

void generateString(int lastElt, std::string & str)
{
    int size = CH_MAX - 1;
    int limit = size;
    int iteration = 0;
    while(limit < lastElt & iteration < SIZE_HASH)
    {
        limit *= size;
        iteration++;
    }
    if (iteration)
        limit /= size;

    str = std::string(SIZE_HASH);
    id = 
    for (int i = 0; i < SIZE_HASH;  i++)
    {
        if(i> interation)
           str[i] = alphabeticsLetters[0];
        else
        {
            int  j = lastElt / limit;
            if (lastElt < CH_MAX)
                int i = lastElt - j * CH_MAX;
        }
        
    }
}

std::string randomString(const size_t length)
{
    std::string str = "";
    str.reserve(length);
    for (int i = 0; i < length; i++)
        str += alphabeticsLetters[rand() % CH_MAX];
    return str;
}

void getHashCodeColision(std::string & str1, std::string& str2,  size_t lenght )
{
    std::hash<std::string> strHash;
    str1 = randomString(lenght);
    str2 = randomString(lenght);
    while ((strHash(str1) != strHash(str2) ))//|| str1 == str2))
    {
      str1 = randomString(lenght);
      str2 = randomString(lenght);
    }
}


int main(int argc, char ** argv)
{

    const auto processorCount = std::thread::hardware_concurrency();
    if(processorCount == 0)
    {
        std::cout<<" INFO: The application will be not multithreaded"<<std::endl;
        std::cout<<" INFO: no multithreading detected on the computer "<<std::endl;
    }
    std::string str1;
    std::string str2;
    size_t lenght = 6;

    auto start = std::chrono::system_clock::now();
    getHashCodeColision(str1, str2, lenght);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "Time get it " << " ints : " << diff.count() << " s\n";
    std::cout << "The collision founded are str1: " << str1 << " str2 : " << str2 << std::endl;
}


*/




