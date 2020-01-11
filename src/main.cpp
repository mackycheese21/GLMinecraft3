#include <iostream>
#include <csignal>

#include "both.h"
#include <execinfo.h>
#include <boost/program_options.hpp>
#include <nbt/nbt.h>

void sig_handler(int sig) {
    //https://stackoverflow.com/questions/9207599/printing-stack-trace-from-a-signal-handler
    int j, nptrs;
    void *buffer[100];
    char **strings;

    nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d addresses\n", nptrs);

    /* The call backtrace_symbols_fd(buffer, nptrs, STDOUT_FILENO)
       would produce similar output to the following: */

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL) {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
        printf("%s\n", strings[j]);

    free(strings);
    exit(sig);
}

int main(int argc, char **argv) {
    for (int i = 0; i < 32; i++) std::signal(i, sig_handler);
//    std::shared_ptr<nbt::nbt> ptr=nbt::make_float(5.0F);
//    std::cout<<ptr->as_float()<<"\n";//5
//    ptr->float_ref()+=5;
//    std::cout<<ptr->as_float()<<"\n";//10
//    std::cin>>ptr->float_ref();
//    ptr->float_ref()*=ptr->as_float();
//    std::cout<<ptr->as_float()<<"\n";//input^2
//
//    ptr=nbt::make_list({nbt::make_float(20),nbt::make_string("hello world")});
//    std::cout<<ptr->to_str("")<<"\n";//[20F,"hello world"]
//    std::cout<<ptr->list_ref()[0]->as_float()<<"\n";//20
//    std::cout<<ptr->list_ref()[1]->as_string()<<"\n";//hello world
//    std::cout<<ptr->list_ref()[1]->as_float()<<"\n";//SEGFAULT
//    if(5>2)return 0;
    boost::program_options::options_description desc("Allowed options");
    desc.add_options()
            ("help", "produce help message")
            ("client", "run GLMC3 client on specified host and port")
            ("server", "run GLMC3 server on specified port")
            ("host", boost::program_options::value<std::string>())
            ("port", boost::program_options::value<int>());

    boost::program_options::variables_map vm;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);
    boost::program_options::notify(vm);
    if (vm.count("help")) {
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("client") && vm.count("server")) {
        std::cout << "Both server and client are not currently supported.\n";
        std::cout << desc << "\n";
        return 1;
    }

    if (vm.count("client")) {
        if (!vm.count("host") && !vm.count("port")) {
            std::cout << "Specify host and port to connect to.\n";
            std::cout << desc << "\n";
            return 1;
        }
        if (!vm.count("host")) {
            std::cout << "Specify host to connect to.\n";
            std::cout << desc << "\n";
            return 1;
        }
        if (!vm.count("port")) {
            std::cout << "Specify port to connect to.\n";
            std::cout << desc << "\n";
            return 1;
        }
        std::cout << "Starting client on host " << vm["host"].as<std::string>() << " and port " << vm["port"].as<int>()
                  << "\n";
        networking::client(vm["host"].as<std::string>(), std::to_string(vm["port"].as<int>()));
        return 0;
    }

    if (vm.count("server")) {
        if (!vm.count("port")) {
            std::cout << "Specify port to run on.\n";
            std::cout << desc << "\n";
            return 1;
        }
        std::cout << "Starting server on port " << vm["port"].as<int>() << "\n";
        networking::server(vm["port"].as<int>());
        return 0;
    }

    std::cout << "Select an option.\n";
    std::cout << desc << "\n";

    return 0;
}
