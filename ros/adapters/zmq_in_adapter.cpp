#include "zmq_in_adapter.h"

#include "rtclock.h"

static void*
zmq_thread(void* arg)
{
  ZmqInAdapter* zmq_adapter = static_cast<ZmqInAdapter*>(arg);
  zmq_adapter->runZMQ();
}


int
main(int argc, char** argv)
{

    ZmqInAdapter zmq_adapter;
    zmq_adapter.init(argc, argv);

    MPI::COMM_WORLD.Barrier();

    pthread_t t;
    pthread_create (&t, NULL, zmq_thread, &zmq_adapter);

    zmq_adapter.runMUSIC();
    pthread_join(t, NULL);

    zmq_adapter.finalize();

}


void
ZmqInAdapter::init(int argc, char** argv)
{
    std::cout << "initializing ZMQ in adapter" << std::endl;

    timestep = DEFAULT_TIMESTEP;
    rtf = DEFAULT_RTF;
    zmq_addr = DEFAULT_ZMQ_ADDR;
    zmq_topic = DEFAULT_ZMQ_TOPIC;

    pthread_mutex_init(&data_mutex, NULL);
    // MUSIC before ZMQ to read the config first!
    initMUSIC(argc, argv);
}


void
ZmqInAdapter::initZMQ(int argc, char** argv)
{
}

void
ZmqInAdapter::initMUSIC(int argc, char** argv)
{
    setup = new MUSIC::Setup (argc, argv);

    setup->config("music_timestep", &timestep);
    setup->config("stoptime", &stoptime);
    setup->config("rtf", &rtf);
    setup->config("zmq_addr", &zmq_addr);
    setup->config("zmq_topic", &zmq_topic);


    MUSIC::ContOutputPort* port_out = setup->publishContOutput ("out");

    comm = setup->communicator ();
    int rank = comm.Get_rank ();       
    int nProcesses = comm.Get_size (); 
    if (nProcesses > 1)
    {
        std::cout << "ERROR: num processes (np) not equal 1" << std::endl;
        comm.Abort(1);
    }

    if (port_out->hasWidth ())
    {
        datasize = port_out->width ();
    }
    else
    {
        std::cout << "ERROR: Port-width not defined" << std::endl;
        comm.Abort (1);
    }
    
    data = new double[datasize]; 
    for (unsigned int i = 0; i < datasize; ++i)
    {
        data[i] = 0.;
    }
         
    // Declare where in memory to put data
    MUSIC::ArrayData dmap (data,
      		 MPI::DOUBLE,
      		 0,
      		 datasize);
    port_out->map (&dmap, 1);
}

void 
ZmqInAdapter::runMUSIC()
{
    std::cout << "running zmq in adapter with update rate of " << 1./timestep << std::endl;
    RTClock clock(timestep / rtf);
  
    runtime = new MUSIC::Runtime (setup, timestep);
    
    for (int t = 0; runtime->time() < stoptime; t++)
    {
        clock.sleepNext(); 

        runtime->tick();
    }

    std::cout << "sensor: total simtime: " << clock.time () << " s" << std::endl;
}

void 
ZmqInAdapter::runZMQ()
{ 
    zmq::context_t context(1);
    //  Connect our subscriber socket
    zmq::socket_t subscriber (context, ZMQ_SUB);
    subscriber.connect(zmq_addr.c_str());
    subscriber.setsockopt(ZMQ_SUBSCRIBE, "", 0); // zmq_topic.c_str(), zmq_topic.size());
    RTClock clock( 1. / (timestep * rtf) );

    // wait until first sensor update arrives
    //s_recvAsVector(subscriber, data);
    // TODO is that needed?

    for (int t = 0; runtime->time() < stoptime; t++)
    {
        std::cout << "recv " << std::endl;
        s_recvAsJson(subscriber, data, datasize);
//        for (int i = 0; i < datasize; ++i)
//            std::cout << data[i] << " ";
//        std::cout << std::endl;

    }
}


void ZmqInAdapter::finalize(){
    runtime->finalize();
    delete runtime;
}



