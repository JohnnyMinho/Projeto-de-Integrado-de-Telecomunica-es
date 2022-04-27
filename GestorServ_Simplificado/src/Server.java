import com.opencsv.CSVWriter;

import javax.management.timer.Timer;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.nio.file.Path;
import java.util.Arrays;
import java.util.Date;

public class Server implements Runnable{

    ServerSocket Server = null;
    String password = "TRY1";
    static int port_used = 4444;
    boolean status = true;
    File csv_direction = new File("/home/johnnyminho/Documents/Projeto Integrado Telecomunicações/Fase B/BaseDados/BaseDados.csv");
   // List

    public void send_message(int tipo){
        System.out.println(Thread.activeCount());
    }

    public void setStatus(boolean status_check){
        status = status_check;
    }

    public void kill_server(){
        try {
            Server.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public void run() {
        try{
            Server = new ServerSocket(port_used);
        }catch(IOException e){
            e.printStackTrace();
            System.out.println("Não consegui criar um servidor.");
        }
        while(status){
            try {
                //System.out.println(Server.getLocalSocketAddress());
                ClientWorker New_Worker = new ClientWorker(Server.accept(),password,csv_direction);
                Thread new_thread = new Thread(New_Worker);
                new_thread.start();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        System.out.println("O Servidor está Desligado.");
    }


    class ClientWorker implements Runnable {

        private final Socket sou_client;
        private String quem_sou;
        private int id = 0;
        private String local;
        private final String Password_to_Match;
        private File BaseDados;
        private boolean Alive = true;
        private boolean authenticated = false;
        private boolean unblocked = false; //Indica que o sistema sensor ainda não está pronto para enviar dados
        private FileWriter outputfile;
        private CSVWriter csv_write;

        //TIPOS DE HEADER
        final byte STOPbyte = 0b00000001; // Tipo - 1, Timestamp - 8 (2bytes - horas, 2 bytes - minutos, 2 bytes - segundos)

        final byte STARTbyte = 0b00000010; // Tipo - 1, Timestamp - 8

        final byte BEGINbyte = 0b00000011; // Tipo - 1

        final byte ENDbyte = 0b00000100; // Tipo - 1

        final byte DATAbyte = 0b00000101; // Tipo - 1, Timestamp - 8, Dados - 17 bytes(5-TEMP,5-Humd,7- Press_atm)

        final byte DISCbyte = 0b00000110; // Tipo - 1, Timestamp - 6 bytes

        final byte ERRORbyte = 0b00000111; // Tipo - 1, Timestamp - 8, Tipo_Erro - 1 byte, Warning - 1 byte (Y / N, caso seja Y, é realizada enviado um packet para reniciar o Sistema Sensor)
        //Caso o error esteja na conexão ao gestor serviços , o problema já é resolvido automáticamente graças ao .accept() visto que é preciso haver uma conexão, senão o próprio progama realiza uma exception visto que não vai conseguir ler da socket
        //O Warning com o valor de Y é principalmente para caso haja falhas graves, ex. na leitura de temperatura (caso por exemplo um dos sensores esteja disconectado) , se for possível implementar uma maneira de encerrar o arduino permanenetemente caso haja n renicizalizações
        //O Warning com o valor de N é principalmente para erros de pouco relevância, mas que valem a pena indicar.

        final byte Authenticationbyte = 0b00001000; // TIPO - 1 byte, Password - 4 bytes , Host name - 7 bytes, Local - 4 bytes -> total 16 bytes
        //Este packet de autenticação para além de permitir a poupança de recursos ao eliminar clientes não autorizados, também permite a eliminicação da necessidade
        //de incidicar o local de onde as amostras são recolhidas.
        //---------------

        ClientWorker(Socket quem_sou_socket, String Password_Recebida,File BaseDados_csv) {
            this.sou_client = quem_sou_socket;
            this.Password_to_Match = Password_Recebida;
            this.BaseDados = BaseDados_csv;
            try {
                this.outputfile= new FileWriter(BaseDados,true);
            } catch (IOException e) {
                e.printStackTrace();
            }
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

        public boolean authentication(DataInputStream input) throws IOException { //Provavelmente não é a melhor maneira de fazer uma autenticação, mas trabalha
            byte message_length_byte = input.readByte();
            int message_length = message_length_byte;
            //System.out.println("Tamanho -> " + message_length);
            boolean authorized = false;
            byte[] authentication_array = new byte[message_length];
            //System.out.println(authentication_array.length);
            if(message_length <= 0){
                while(message_length <= 0){
                    message_length = input.readInt();
                    System.out.println("NO MESSAGE RECEIVED");
                }
            }

            if(message_length > 0){
                //System.out.println("HELLO");
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
                byte[] local_sensor = Arrays.copyOfRange(authentication_array,12,16);
                char[] password_to_match = Password_to_Match.toCharArray();
                byte[] password_byte = new byte[password_to_match.length];
                char[] client_name_char = new char[client_name.length];
                char[] client_local = new char[local_sensor.length];

                for(int i = 0 ; i<password_to_match.length; i++ ){
                    password_byte[i] = (byte)password_to_match[i];
                }
                for(int i = 0 ; i<client_name.length; i++){
                    client_name_char[i] = (char)client_name[i];
                }
                for(int i = 0 ; i<local_sensor.length; i++){
                    client_local[i] = (char)local_sensor[i];
                }

                if(Arrays.equals(newArray, password_byte)){
                    System.out.println("Autenticado");
                    String tmp = new String(client_name_char);
                    String tmp2 = new String(local_sensor);
                    authenticated = true;
                    setAuthenticated(authenticated);
                    setHostName(tmp);
                    System.out.println("Eu sou um novo cliente com o nome: " + tmp + " num local com o código postal " + tmp2);
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

            switch(tipo){
                case 1:
                        try {
                            DataOutputStream  toclient = new DataOutputStream((sou_client.getOutputStream()));
                            char num = '1';
                            toclient.write(num);
                            toclient.flush();
                            toclient.write(ENDbyte);
                            toclient.flush();
                            setStatus(false);
                        } catch (IOException e) {
                            e.printStackTrace();
                        }

                    break;
                case 2:
                    try {
                        DataOutputStream  toclient = new DataOutputStream((sou_client.getOutputStream()));
                        char num = '1';
                        toclient.write(num);
                        toclient.flush();
                        toclient.write(BEGINbyte);
                        toclient.flush();
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
            System.out.println("Novo cliente está a correr, mas ainda tem de ser autenticado");

            DataInputStream fromclient = null;
            DataOutputStream toclient = null;

            try {
                fromclient = new DataInputStream((sou_client.getInputStream()));
                toclient = new DataOutputStream((sou_client.getOutputStream()));
                authentication(fromclient);
            } catch (IOException e) {
                e.printStackTrace();
            }

            if(authenticated){
                //System.out.println("HELLO");
                send_message_client(2);
            }
            while (Alive && authenticated){
                Date current_time = new Date();

                try {
                    sou_client.setSoTimeout(45000);
                } catch (SocketException e) {
                    e.printStackTrace();
                }
                try {
                    byte message_length_byte = fromclient.readByte();
                    int array_received_size = message_length_byte;
                    if(unblocked){
                        long time_after_new_packet = current_time.getTime();
                    }
                    if (array_received_size > 0){
                        byte[] arrayreceived = new byte[array_received_size];
                        fromclient.readFully(arrayreceived);
                        switch (arrayreceived[0]){
                            case STARTbyte:
                                    unblocked = true; //Serve como mensagem de que vai começar a enviar mensagens
                                    System.out.println("Cliente " + quem_sou + " está pronto para enviar dados" );
                                break;
                            case STOPbyte:
                                    System.out.println("Cliente " + quem_sou + " não vai enviar mais mensagens");
                                    setStatus(false);
                                break;
                            case ERRORbyte:
                                char[] time_received_error= {(char) arrayreceived[1], (char)arrayreceived[2],(char)arrayreceived[3],(char)arrayreceived[4],(char)arrayreceived[5],(char)arrayreceived[6],(char)arrayreceived[7],(char)arrayreceived[8]};
                                char error_type = (char)arrayreceived[9];
                                char error_level = (char)arrayreceived[10];
                                String time = String.valueOf(time_received_error);
                                System.out.println("Client: " + quem_sou + " teve um erro do tipo " + error_type + " com o nível " + error_level + " pelas: " + time);
                                if (error_level == 'Y') {
                                    send_message_client(1);
                                }
                                break;
                            case DATAbyte:
                                csv_write = new CSVWriter(outputfile);
                                // Tipo - 1, Timestamp - 8, Dados - 17 bytes(5-TEMP,5-Humd,7- Press_atm)
                                char[] time_received= {(char) arrayreceived[1], (char)arrayreceived[2],(char)arrayreceived[3],(char)arrayreceived[4],(char)arrayreceived[5],(char)arrayreceived[6],(char)arrayreceived[7],(char)arrayreceived[8]};
                                char[] Temp = {(char)arrayreceived[9],(char)arrayreceived[10],(char)arrayreceived[11],(char)arrayreceived[12],(char)arrayreceived[13]};
                                char[] Humd = {(char)arrayreceived[14],(char)arrayreceived[15],(char)arrayreceived[16],(char)arrayreceived[17],(char)arrayreceived[18]};
                                char[] Press = {(char)arrayreceived[19],(char)arrayreceived[20],(char)arrayreceived[21],(char)arrayreceived[22],(char)arrayreceived[23],(char)arrayreceived[24],(char)arrayreceived[25]};
                                String time_stamp = String.valueOf(time_received);
                                String string_temp = String.valueOf(Temp);
                                String string_humd = String.valueOf(Humd);
                                String string_Press = String.valueOf(Press);
                                System.out.println(time_stamp);
                                String[] data_input ={ time_stamp ,string_temp , string_humd , string_Press + "\n"};
                                csv_write.writeNext(data_input);
                                //Parte de processamento
                                csv_write.flush();
                                break;
                            default:
                                System.out.println("Header não reconhecido");
                                break;
                        }
                    }
                    if (Thread.interrupted()) {
                        return;
                    }
                } catch (SocketException e) {
                    e.printStackTrace();
                    System.out.println("Something is wrong with the socket");
                }catch (SocketTimeoutException e){
                    e.printStackTrace();
                    System.out.println("SOCKET DEU TIMEOUT, a conexão vai ser terminada");
                    send_message_client(1);
                    disconnect();
                }
                catch(EOFException e){
                    e.printStackTrace();
                    try {
                        if(fromclient.read() == -1) {
                            System.out.println("Conexão Cliente-Servidor falhou. O Handler vai ser desligado");
                            csv_write.close();
                            setStatus(false);
                        }
                    } catch (IOException ex) {
                        ex.printStackTrace();
                    }
                } catch (IOException e) {
                    e.printStackTrace();
                }
            }
            System.out.println("Acabei o Run, Adeus");
            try {
                sou_client.close();
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    }
}