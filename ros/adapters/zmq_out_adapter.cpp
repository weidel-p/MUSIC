#include "zmq_out_adapter.h"

#include "rtclock.h"

int
main(int argc, char** argv)
{

    ZmqOutAdapter zmq_adapter;
    zmq_adapter.init(argc, argv);

    MPI::COMM_WORLD.Barrier();

    zmq_adapter.runMUSIC();

    zmq_adapter.finalize();

}

void
ZmqOutAdapter::init(int argc, char** argv)
{
    std::cout << "initializing ZMQ out adapter" << std::endl;

    timestep = DEFAULT_TIMESTEP;
    rtf = DEFAULT_RTF;
    zmq_addr = DEFAULT_ZMQ_ADDR;
    zmq_topic = DEFAULT_ZMQ_TOPIC;


    pthread_mutex_init(&data_mutex, NULL);

    // MUSIC before ZMQ to read the config first!
    initMUSIC(argc, argv);
    initZMQ(argc, argv);
}


void
ZmqOutAdapter::initZMQ(int argc, char** argv)
{
}

void
ZmqOutAdapter::initMUSIC(int argc, char** argv)
{
    setup = new MUSIC::Setup (argc, argv);

    setup->config("stoptime", &stoptime);
    setup->config("music_timestep", &timestep);
    setup->config("rtf", &rtf);
    setup->config("zmq_addr", &zmq_addr);
    setup->config("zmq_topic", &zmq_topic);

    
    MUSIC::ContInputPort* port_in = setup->publishContInput ("in"); //TODO: read portname from file
    
    comm = setup->communicator ();
    int rank = comm.Get_rank ();       
    int nProcesses = comm.Get_size (); 
    if (nProcesses > 1)
    {
        std::cout << "ERROR: num processes (np) not equal 1" << std::endl;
        comm.Abort(1);
    }

    int width = 0;
    if (port_in->hasWidth ())
    {
        width = port_in->width ();
    }
    else
    {
        std::cout << "ERROR: Port-width not defined" << std::endl;
        comm.Abort (1);
    }
    
    datasize = width;
    data = new double[datasize]; //+1 for the leading zero needed for unspecified fiels in the message 
    for (unsigned int i = 0; i < datasize; ++i)
    {
        data[i] = 0.;
    }
    // Declare where in memory to put data
    MUSIC::ArrayData dmap (data,
      		 MPI::DOUBLE,
      		 0,
      		 datasize);
    port_in->map (&dmap, 0., 1, false);
}

void
ZmqOutAdapter::sendZMQ ()
{
}

void 
ZmqOutAdapter::runMUSIC()
{
    std::cout << "running zmq out adapter with update rate of " << timestep << std::endl;
    RTClock clock(timestep / rtf);

    zmq::context_t context(1);

    //  Connect our subscriber socket
    zmq::socket_t pub (context, ZMQ_PUB);
    pub.bind(zmq_addr.c_str());
 
    runtime = new MUSIC::Runtime (setup, timestep);
    
    for (int t = 0; runtime->time() < stoptime; t++)
    {
        clock.sleepNext();
	    pthread_mutex_lock (&data_mutex);

        std::ostringstream message;
        message << zmq_topic;
        for (unsigned int i = 0; i < datasize; ++i){
            message << " " << data[i];
        }

        s_send (pub, message.str());

	    pthread_mutex_unlock (&data_mutex);
        runtime->tick();
    }

    std::cout << "out: total simtime: " << clock.time () << " s" <<  std::endl;
}

void ZmqOutAdapter::finalize()
{

    runtime->finalize();
    delete runtime;
}

