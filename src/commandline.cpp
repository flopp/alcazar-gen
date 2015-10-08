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

#include <boost/program_options.hpp>
#include <iostream>
#include "commandline.h"

namespace po = boost::program_options;

void usage(char* program, const po::options_description& desc)
{
    std::cout 
        << "Usage: " << program << " [OPTIONS]... [WIDTH HEIGHT]\n"
        << desc << std::endl;
}

bool parseCommandLine(int argc, char** argv, Options& options)
{
    po::options_description desc("Allowed options");
    desc.add_options()
        ("help", "Display this help message")
        ("seed", po::value<unsigned int>(), "Set random seed")
        ("solve", "Solve generated puzzle")
    ;

    po::options_description hidden("Hidden options");
    hidden.add_options()
        ("dimensions", po::value<std::vector<int>>(), "dimensions")
    ;

    po::positional_options_description p;
    p.add("dimensions", -1);

    po::options_description cmdline_options;
    cmdline_options.add(desc).add(hidden);
    
    try
    {        
        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).options(cmdline_options).positional(p).run(), vm);
        po::notify(vm);
        
        if (vm.count("help"))
        {
            usage(argv[0], desc);
            return false;
        }
        
        if (vm.count("dimensions"))
        {
            const std::vector<int>& dim = vm["dimensions"].as<std::vector<int>>();
            if (dim.size() != 2 || dim[0] < 2 || dim[1] < 2)
            {
                throw std::invalid_argument("bad dimensions (WIDTH and HEIGHT must be >= 2)");
            }
            
            options.width = dim[0];
            options.height = dim[1];
        }
        else
        {
            throw std::invalid_argument("missing dimensions");
        }
        
        if (vm.count("seed"))
        {
            options.seed = vm["seed"].as<unsigned int>();
        }
        
        options.solve = vm.count("solve") > 0;
        
        return true;
    }
    catch (std::exception& e) 
    {
        std::cout << "Error: " << e.what() << "\n\n";
        usage(argv[0], desc);
        return false;
    }
}
