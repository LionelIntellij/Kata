#include <string>
#include <iostream>
#if _WIN32
	#define EXE ".exe"
#endif



int GetNthRoot(double number, int n)
{
  /**\brief get the nth root
   * \arg number: the final value
   *       n: the exposant
   * \return  the nth root number
   **/
	int iNumber = static_cast<int>(number);
	//particular case
	if (iNumber == 1 )
		return 1;

	//particular case
	if (iNumber == 0)
		return 0;

	/**  Newton method x_k+1 = x_k - f(x_k)/fprime(x_k)
         f(x_k) = x_k^n - Number
		 fprime = nx_k^(n-1)
         ===> x_k+1 = x_k  - (x_k^n - number)/nx_k^(n-1) 
		 We can rewrite the formula like that:
		 ===> x_k+1 = ((n-1) x_k+ number/ x_k^(n-1))/n
		 we note x_k+1 = x_k and x_k = x_kprev
	**/
	int x_kprev = iNumber + 1;
	for (int x_k = iNumber; x_k < x_kprev;)
	{
		x_k = (x_k + (n - 1) * x_kprev)/n;
		x_kprev = x_k;
		x_k = iNumber;
		for (int c = n - 1; c > 0; --c)
			x_k /= x_kprev;
	}
	return x_kprev;
}

void show_usage(const std::string& filename)
{
	std::cout << "Usage: Compute nth root value. Please enter a number and a exposant n. \
		\n Example : nthRoot"<< EXE <<" -number 81 -n 4" << std::endl;
}

int main(int argc, char*argv[])
{
	double number = 0;
	int  n = 0;

	for (int i = 1; i < argc; ++i)
	{
		std::string arg = argv[i];
		if ((arg == "-h") || (arg == "--help"))
		{
			show_usage(argv[0]);
			return 0;
		}
		else if (arg == "-n")
		{
			if (i + 1 < argc)
			{
				try
				{
					n = std::stoi(argv[++i]);
				}
				catch (std::exception& e)
				{
					(void)e;
					std::cerr << "Error: -n option argument is not correct" << std::endl;
					show_usage(argv[0]);
					return 1;
				}
			}
			else
			{
				std::cerr << "Error: -n option requires one argument." << std::endl;
				return 1;
			}
		}
		else if (arg == "-number")
		{
			if (i + 1 < argc)
			{
				try
				{
					number = std::stold(argv[++i]);
				}
				catch (std::exception& e)
				{
					(void)e;
					std::cerr << "Error: -number option argument is not correct" << std::endl;
					show_usage(argv[0]);
					return 1;
				}
			}
			else
			{
				std::cerr << "Error: -n option requires one argument." << std::endl;
				return 1;
			}
		}
	}
	if (n < 0)
	{
		std::cerr << "Error : The entered exposant must be positive " << std::endl;
		std::cerr << "The entered exposant is " << n << std::endl;
		show_usage(argv[0]);
		return 1;
	}

	if (number < 0)
	{
		std::cerr << "Error: The number must be positive " << std::endl;
		std::cerr << "The entered number is " << number << std::endl;
		show_usage(argv[0]);
		return 1;
	}
	int nthRoot = GetNthRoot(number, n);
	std::cout <<"Info: Your Result : " << nthRoot << std::endl;
	return 0;
}

