import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.Arrays;
import java.util.stream.Stream;

public class Interface {

    // GUI-----
    private JPanel Interface_view;
    private JTextArea Incoming_text_clients;
    private JTextArea Connected_clients;
    private JButton conectarNovoSistemaSensorButton;
    private JButton pararUmSistemaSensorButton;
    private JButton reniciarSistemaSensorButton;
    private JButton desligarGestorServiçoButton;
    private JSpinner Client_Selector;
//
    //SERVIDOR

    //LIGADOS À INTERFACE

    //-------------------

    ServerSocket server;
    {
        try {
            server = new ServerSocket(4444);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    static Thread [] Thread_array = new Thread[20];
    static ClientWorker[] ClientHandlerArray = new ClientWorker[20];
    String password = "TRY1";
    boolean new_data = false;
//
    static int port_used = 4444;
    int N_clientes = 0;
    boolean new_text = false;

   /* public boolean initialize_server(int port_used) throws IOException {
        boolean sucess = false;
        while(!sucess) {
            try {
                server = new ServerSocket(port_used);
                server.accept();
            } catch (IOException e) {
                System.out.println("Não consigo ler a partir desta porta");
                System.out.println("Tente com outra porta (maior que 1024)");
            }finally{
                sucess = true;
            }
        }
        return sucess;
    }*/

   public void searchforclients(int Numero_clientes) throws IOException {
        boolean status = false;
        int n_max_attempts = 0;
        server.setSoTimeout(10000);
        while (!status && n_max_attempts != 5){
            try{
                ClientHandlerArray[Numero_clientes] = new ClientWorker(server.accept(), password, Incoming_text_clients);
                status = true;
            } catch (IOException e){
                System.out.println("FAILED TO FIND CLIENTS");
              /*  if(Thread_array[Numero_clientes] != null) {
                    Thread_array[Numero_clientes].interrupt();
                }*/
                System.out.println(e);
                String to_text = "Attempt number " + (n_max_attempts+1) + " \n";
                Incoming_text_clients.append(to_text);
                n_max_attempts++;
                status = false;
            }finally{
                if(status){
                    System.out.println("HELLO5");
                    Thread_array[Numero_clientes] = new Thread(ClientHandlerArray[Numero_clientes]);
                    Thread_array[Numero_clientes].start();
                }
            }
        }
    }

    public void ShowAllClients(ClientWorker[] c, int Num_Clientes){
        for(int i = 0;i<Num_Clientes;i++) {
            if(c[i] != null) {
                System.out.print("IP Cliente" + i + ": ");
                System.out.println(c[i].show_ip());
            }
        }
    }

    public void STOPSERVER(boolean yesorno){
        if(yesorno){
            try {
                server.close();
                for(int i = 0; i<ClientHandlerArray.length;i++){
                    ClientHandlerArray[i].disconnect();
                    Thread_array[i].interrupt();
                }
                System.exit(1);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }

    public boolean Server_Status(){
        boolean status = false;
        if(this.server.isClosed()){
            status = true;
            System.out.println("LOL");
        }
        return status;
    }

    public boolean DisconectSensor(Thread cliente, ClientWorker c){
        boolean sucess_t = false;
        boolean sucess_c = false;
        boolean sucess = false;
        if(c.disconnect()){
            sucess_c = true;
        }
        cliente.interrupt();
        if(cliente.isInterrupted()){
            sucess_t = true;
        }
        if(sucess_c && sucess_t){
            sucess = true;
        }
        return sucess;
    }

    public Interface(){
        conectarNovoSistemaSensorButton.addActionListener(new ConnectNewClient());
        desligarGestorServiçoButton.addActionListener(new KillServer());
    }

    public void add_text(String text_To_append){
       // Incoming_text_clients.append();
    }


    /*static public void start_server_static(){
        Interface main_interface = new Interface();
        try {
            main_interface.initialize_server(port_used);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }*/

    private class ConnectNewClient implements ActionListener{

        @Override
        public void actionPerformed(ActionEvent e) {
            if(N_clientes < 20){
                try {
                    searchforclients(N_clientes);
                } catch (IOException ex) {
                    ex.printStackTrace();
                }
            }
        }
    }

    private class KillServer implements ActionListener{
        @Override
        public void actionPerformed(ActionEvent e) {
           STOPSERVER(true);
        }
    }


    public static void main(String[] args) {
        JFrame frame = new JFrame("Interface");
        frame.setContentPane(new Interface().Interface_view);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);
        //start_server_static();
    }


    // Client Handler

    class ClientWorker implements Runnable {

        private Socket sou_client;
        private String quem_sou;
        private String local;
        private String Password_to_Match;
        private JTextArea to_write;
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

        ClientWorker(Socket quem_sou, String Password_Recebida,JTextArea write_here) {
            this.sou_client = quem_sou;
            this.Password_to_Match = Password_Recebida;
            this.to_write = write_here;
        }

        public void setStatus(boolean status) {
            Alive = status;
        }

        public void setHostName(String my_name){
            this.quem_sou = my_name;
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
                byte[] client_name = Arrays.copyOfRange(authentication_array, 5,13);
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
                    to_write.append(tmp);
                    authenticated = true;
                    setAuthenticated(authenticated);
                    setHostName(Arrays.toString(client_name));
                }
            }
            return authorized;
        }

        public String show_ip() {
            return sou_client.getLocalAddress().toString();
        }

        public boolean send_message_client(byte[] mensagem) {
            boolean sucess = true;
            return sucess;
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

            to_write.append("Novo cliente está a correr, mas ainda tem de ser autenticado");

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
                        switch(arrayreceived[0]){
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
                    if(Thread.interrupted()){
                        return;
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }

            }
        }
    }
}
