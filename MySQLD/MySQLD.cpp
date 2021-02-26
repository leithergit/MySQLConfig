// MySQLD.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
  
    std::ofstream ofs("out.txt");
    for (int i = 0;i < argc;i ++)
    {
        ofs <<"argv["<<i<<"] = "<< argv[i] << std::endl;
    }
   
    ofs.close();
    return 0;
}


