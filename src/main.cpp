/*******************************************************************************
* alcazar-gen
*
* Copyright (c) 2015 Florian Pigorsch
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*******************************************************************************/

#include <boost/lexical_cast.hpp>
#include <iostream>
#include "board.h"
#include "generator.h"

void usage(char* program)
{
    std::cout << "USAGE: " << program << " WIDTH HEIGHT\n"
    << "Generate a random Alcazar puzzle with size WIDTH x HEIGHT" << std::endl;
}

bool parseCommandLine(int argc, char** argv, int& width, int& height)
{
    if (argc != 3)
    {
        return false;
    }
    
    try
    {
        width = boost::lexical_cast<int>(std::string(argv[1]));
    }
    catch (const boost::bad_lexical_cast& e)
    {
        std::cout << "ERROR: '" << argv[1] << "' is not a number." << std::endl;
        return false;
    }
    
    try
    {
        height = boost::lexical_cast<int>(std::string(argv[2]));
    }
    catch (const boost::bad_lexical_cast& e)
    {
        std::cout << "ERROR: '" << argv[2] << "' is not a number." << std::endl;
        return false;
    }
    
    if (width < 1 || height < 1)
    {
        std::cout << "ERROR: WIDTH and HEIGHT must both be greater or equal than 1." << std::endl;
        return false;
    }
    
    return true;
}


int main(int argc, char** argv)
{
    int width = 0;
    int height = 0;
    if (!parseCommandLine(argc, argv, width, height))
    {
        usage(argv[0]);
        return 1;
    }
    
    const Board b = generate(width, height);
    std::cout << b << std::endl;
    
    std::cout << "Recomputing solution..." << std::endl;
    std::tuple<bool, bool, Path> solution = b.solve();
    if (std::get<0>(solution))
    {
        std::cout << "Board is solvable" << std::endl;
        
        if (std::get<1>(solution))
        {
            std::cout << "Board is uniquely solvable" << std::endl;
        }
        else
        {
            std::cout << "Board is NOT uniquely solvable" << std::endl;
        }
        
        std::cout << "Solution:" << std::endl;
        b.print(std::cout, std::get<2>(solution));
    }
    else
    {
        std::cout << "Board is NOT solvable" << std::endl;
    }
    
    return 0;
}
