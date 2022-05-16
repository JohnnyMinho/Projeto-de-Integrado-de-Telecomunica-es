import java.io.*;
import java.net.Socket;

public class ClientWorker2 implements Runnable{

    private Socket sou_client;
    private String Password_to_Match;
    private boolean Alive = true;


    ClientWorker2(Socket quem_sou,String Password_Recebida){
        this.sou_client = quem_sou;
        this.Password_to_Match = Password_Recebida;
    }

    public void setStatus(boolean status){
        Alive = status;
    }

    public String show_ip(){
        return sou_client.getLocalAddress().toString();
    }

    public boolean send_message_client(byte[] mensagem){
        boolean sucess = true;
        return sucess;
    }

    public boolean disconnect(){
        boolean sucess = false;
        try {
            sou_client.close();
        } catch (IOException e) {
            e.printStackTrace();
            System.out.println("Não consegui disconectar o cliente");
        }
        if(sou_client.isClosed()){
            sucess = true;
        }
        setStatus(sucess);
        return sucess;
    }

    @Override
    public void run() {
        while(Alive) {
            try {
                DataInputStream fromclient = new DataInputStream(new BufferedInputStream(sou_client.getInputStream()));
                DataOutputStream toclient = new DataOutputStream(new BufferedOutputStream(sou_client.getOutputStream()));
                if(fromclient.available()>0){

                }
            } catch (IOException e) {
                e.printStackTrace();
            }

        }
    }

}
