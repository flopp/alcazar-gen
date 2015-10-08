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

#include <iostream>
#include "board.h"
#include "commandline.h"
#include "generator.h"


int main(int argc, char** argv)
{
    Options options;
    if (!parseCommandLine(argc, argv, options))
    {
        return 1;
    }
    
    const Board b = generate(options.width, options.height, options.seed);
    std::cout << b << std::endl;
    
    if (options.solve)
    {
        std::cout << "Computing solution..." << std::endl;
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
    }
        
    return 0;
}
