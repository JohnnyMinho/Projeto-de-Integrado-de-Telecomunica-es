import java.net.ServerSocket;
public class CreateThread implements Runnable{ //Contém o que cada Thread to TCP server deve fazer, ou seja

    ClientWorker slave;
    CreateThread(ClientWorker who_is_slave){
        this.slave = who_is_slave;
    }

    @Override
    public void run() {

    }
}
