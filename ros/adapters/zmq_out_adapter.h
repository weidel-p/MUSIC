#include <iostream>
#include <map>

#include "zhelpers.hpp"

#include <music.hh>
#include <mpi.h>

#include "sys/time.h"

#include "jsoncpp/json/json.h"
#include <fstream>
#include <pthread.h>

#define DEBUG_OUTPUT false 


const double DEFAULT_TIMESTEP = 1e-3;
const double DEFAULT_RTF = 1.0;
const std::string DEFAULT_ZMQ_ADDR = "tcp://*:5555";
const std::string DEFAULT_ZMQ_TOPIC = "out";

class ZmqOutAdapter
{
    public:
        void init(int argc, char** argv);
        void runMUSIC();
        void finalize();

    private:

        MPI::Intracomm comm;
	    MUSIC::Setup* setup;
        MUSIC::Runtime* runtime;
        double stoptime;
        unsigned int datasize;

    	pthread_mutex_t data_mutex;
        double* data;

        double timestep;
        double rtf;
        std::string zmq_addr;
        std::string zmq_topic;


        void initZMQ(int argc, char** argv);
        void initMUSIC(int argc, char** argv);
	    void sendZMQ();

};
