import java.net.Socket;

public class ClientWorker implements Runnable{
    private Socket sou_client;
    private String Password_to_Match;

    ClientWorker(Socket quem_sou,String Password_Recebida){
        this.sou_client = quem_sou;
        this.Password_to_Match = Password_Recebida;
    }

    @Override
    public void run() {

    }
}
