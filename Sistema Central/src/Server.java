import javax.swing.*;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.net.SocketException;
import java.util.Arrays;

public class Server implements Runnable{


    @Override
    public void run() {

    }

    class ClientWorker implements Runnable {

        private Socket sou_client;
        private String quem_sou;
        private int id = 0;
        private String local;
        private String Password_to_Match;
        private JTextArea to_write;
        private JComboBox onde_me_colocar;
        private boolean Alive = true;
        private boolean authenticated = false;

        //TIPOS DE HEADER
        final byte STOPbyte = 0b0000001;
        final byte STARTbyte = 0b0000010;
        final byte BEGINbyte = 0b000011;
        final byte ENDbyte = 0b00000100;
        final byte DATAbyte = 0b0000101;
        final byte DISCbyte = 0b0000110;
        final byte ERRORbyte = 0b0000111;
        final byte Authenticationbyte = 0b00001000; // TIPO - 1 byte, Password - 4 bytes , Host name - 7 bytes, Local - 4 bytes -> total 15 bytes
        //---------------

        ClientWorker(Socket quem_sou_socket, String Password_Recebida,JTextArea write_here, int numero_c, JComboBox Put_me_here) {
            this.sou_client = quem_sou_socket;
            this.Password_to_Match = Password_Recebida;
            this.to_write = write_here;
            this.id = numero_c;
            this.onde_me_colocar = Put_me_here;
        }

        public void setStatus(boolean status) {
            Alive = status;
        }

        public void setHostName(String my_name){
            this.quem_sou = my_name;
        }

        public int getId(){
            return this.id;
        }

        public void setAuthenticated(boolean status){
            this.authenticated = status;
        }

        public boolean getAuthenticated(){
            return authenticated;
        }
        public String getClientName(){
            return this.quem_sou;
        }

        public boolean authentication(DataInputStream input) throws IOException {
            int message_length = input.readInt();
            System.out.println("Tamanho -> " + message_length);
            boolean authorized = false;
            byte[] authentication_array = new byte[message_length];
            System.out.println(authentication_array.length);
            if(message_length <= 0){
                while(message_length <= 0){
                    message_length = input.readInt();
                    System.out.println("NO MESSAGE RECEIVED");
                }
            }

            if(message_length > 0){
                System.out.println("HELLO");
                input.readFully(authentication_array,0,message_length);
                char[] array_char = new char[authentication_array.length];
                for(int i = 0; i<authentication_array.length;i++){
                    array_char[i] = (char) authentication_array[i];
                    System.out.println(array_char[i]);
                }
            }

            if(authentication_array[0] == Authenticationbyte){
                byte[] newArray = Arrays.copyOfRange(authentication_array, 1, 5);
                byte[] client_name = Arrays.copyOfRange(authentication_array, 5,12);
                char[] password_to_match = Password_to_Match.toCharArray();
                byte[] password_byte = new byte[password_to_match.length];
                char[] client_name_char = new char[client_name.length];

                for(int i = 0 ; i<password_to_match.length; i++ ){
                    password_byte[i] = (byte)password_to_match[i];
                }
                for(int i = 0 ; i<client_name.length; i++){
                    client_name_char[i] = (char)client_name[i];
                }

                if(Arrays.equals(newArray, password_byte)){
                    System.out.println("Autenticado");
                    String tmp = new String(client_name_char);
                    to_write.append(tmp + "\n");
                    authenticated = true;
                    setAuthenticated(authenticated);
                    setHostName(tmp);
                    //onde_me_colocar.addItem(new Interface.Selected_Item(id,quem_sou));
                }else{
                    System.out.println("Sem autenticação válida, vai ser disconectado");
                    sou_client.close();
                    authenticated = false;
                    setAuthenticated(authenticated);
                }
            }
            return authorized;
        }

        public String show_ip() {
            return sou_client.getLocalAddress().toString();
        }

        public void send_message_client(int tipo) {
            DataOutputStream toclient = null;
            try {
                toclient = new DataOutputStream((sou_client.getOutputStream()));
            } catch (IOException e) {
                e.printStackTrace();
            }
            switch(tipo){
                case 1:
                    try {
                        toclient.writeInt(1);
                        toclient.write(ENDbyte);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    break;
                case 2:
                    try {
                        toclient.writeInt(1);
                        toclient.write(BEGINbyte);
                    } catch (IOException e) {
                        e.printStackTrace();
                    }
                    break;
            }
        }

        public boolean disconnect() {
            boolean sucess = false;
            try {
                sou_client.close();
            } catch (IOException e) {
                e.printStackTrace();
                System.out.println("Não consegui disconectar o cliente");
            }
            if (sou_client.isClosed()) {
                sucess = true;
            }
            if(sucess){
                setStatus(false);
            }
            return sucess;
        }

        @Override
        public void run() {
            byte[] arrayreceived;
            System.out.println("HELLO2");

            to_write.append("Novo cliente está a correr, mas ainda tem de ser autenticado" +"\n");

            DataInputStream fromclient = null;
            DataOutputStream toclient = null;

            try {
                fromclient = new DataInputStream((sou_client.getInputStream()));
                toclient = new DataOutputStream((sou_client.getOutputStream()));
                authentication(fromclient);
            } catch (IOException e) {
                e.printStackTrace();
            }

            while (Alive && authenticated) {
                try {
                    if (fromclient.available() > 0) {
                        arrayreceived = fromclient.readAllBytes();
                        switch (arrayreceived[0]) {
                            case STARTbyte:

                                break;
                            case STOPbyte:

                                break;
                            case ERRORbyte:

                                break;
                            case DATAbyte:

                                break;
                            default:

                                break;
                        }
                        /*if(fromclient[0] == DATAbyte){

                        }*/
                    }
                    if (Thread.interrupted()) {
                        return;
                    }
                } catch (SocketException e) {
                    e.printStackTrace();
                    System.out.println("Something is wrong with the socket");
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            System.out.println("Acabei o Run, Adeus");
        }
    }

}
