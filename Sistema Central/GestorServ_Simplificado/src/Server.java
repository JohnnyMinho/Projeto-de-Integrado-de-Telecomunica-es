import com.opencsv.CSVWriter;

import javax.management.timer.Timer;
import java.io.*;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketException;
import java.net.SocketTimeoutException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.sql.*;
import java.util.Arrays;
import java.util.Date;
import java.sql.DriverManager;
import java.sql.Connection;
import java.sql.ResultSet;
import java.sql.Statement;

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
        private int entrada = 0;
        private String latitude;
        private String longitude;
        private final String Password_to_Match;
        private File BaseDados;
        private boolean Alive = true;
        private boolean authenticated = false;
        private boolean unblocked = false; //Indica que o sistema sensor ainda não está pronto para enviar dados
        private FileWriter outputfile;
        private CSVWriter csv_write;


        //TIPOS DE HEADER
        final byte STOPbyte = 0b00000001; // Tipo - 1, Tipo - 1(DHT) , 2(BMP), 3(Geral) , Caso seja usado pelo Gestor de Serviços tem esta arquitetura, caso seja usado pelo Gateway, só envia um byte com o tipo

        final byte STARTbyte = 0b00000010; // Tipo - 1, Sensores (tamanho variavel) , o id dos sensores vão vir sobre a forma de um número, ver lista associada, caso seja para adicionar algum tipo de sensor
        //1- DHT11, 2- BMP280 , caso esteja a ocorrer algum erro, um administrador deve ter a possibilidade de parar o funcionamento do mesmo
        final byte BEGINbyte = 0b00000011; // Tipo - 1

        final byte ENDbyte = 0b00000100; // Tipo - 1

        final byte DATAbyte = 0b00000101; // Tipo - 1, Timestamp - 19, Dados - 17 bytes(5-TEMP,5-Humd,7- Press_atm)

        final byte DISCbyte = 0b00000110; // Tipo - 1, Timestamp - 6 bytes

        final byte ERRORbyte = 0b00000111; // Tipo - 1, Timestamp - 8, Tipo_Erro - 1 byte, Warning - 1 byte (Y / N, caso seja Y, é realizada enviado um packet para reniciar o Sistema Sensor)
        //Caso o error esteja na conexão ao gestor serviços , o problema já é resolvido automáticamente graças ao .accept() visto que é preciso haver uma conexão, senão o próprio progama realiza uma exception visto que não vai conseguir ler da socket
        //O Warning com o valor de Y é principalmente para caso haja falhas graves, ex. na leitura de temperatura (caso por exemplo um dos sensores esteja disconectado) , se for possível implementar uma maneira de encerrar o arduino permanenetemente caso haja n renicizalizações
        //O Warning com o valor de N é principalmente para erros de pouco relevância, mas que valem a pena indicar.

        final byte Authenticationbyte = 0b00001000; // TIPO - 1 byte, Password - 4 bytes , Host name - 7 bytes, Local - 4 bytes -> total 16 bytes
        //Este packet de autenticação para além de permitir a poupança de recursos ao eliminar clientes não autorizados, também permite a eliminicação da necessidade
        //de incidicar o local de onde as amostras são recolhidas.
        //---------------

        final byte RESTARTbyte = 0b00001001; //Tipo - 1 , what_to_start (1 - DHT11, 2 - BMP, 3 - Geral), este é apenas enviado pelo gestor de serviços para reniciar um sensor em especifico ou o sistema sensor em geral;

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

        private void updatesql(int i,String timestamp, String quem_sou, String latitude2, String temp, String humd, String press,String longitude2){
            String url = "jdbc:mysql://localhost:3306/tabeladb";
            String user = "root";
            String password = "Sins@741";
            String sql = "select * from tabeladb";
            String columnLabel = "";
            String putQuery = "INSERT INTO tabeladb(ID,Timestamp,Latitude,Longitude,Temperatura,Humidade,Pressão)VALUES(?, ?, ?, ?, ?, ?,?)";

            Float temp_send = null;
            Float humd_send = null;
            Float press_send = null;
            if(!temp.contains("*") ){
                temp_send = Float.parseFloat(temp);
            }
            if(!humd.contains("*")){
                humd_send = Float.parseFloat(humd);
            }
            if(!press.contains("-")){
                press_send = Float.parseFloat(press);
            }
            try{
                Class.forName("com.mysql.cj.jdbc.Driver");
            } catch(ClassNotFoundException e) {
                e.printStackTrace();
            }
            try {

//                Connection connection = DriverManager.getConnection( url, user, password);

                //Statement statement = connection.createStatement();

                try(Connection con = DriverManager.getConnection(url, user, password);
                    Statement statement = con.createStatement();
                ){
                    PreparedStatement pstmt = con.prepareStatement(putQuery);
                    //pstmt.setInt(1,i);
                    pstmt.setString(1, quem_sou);
                    pstmt.setString(2, timestamp);
                    pstmt.setString(3, latitude2);
                    pstmt.setString(4,longitude2);
                    if(temp_send!=null) {
                        pstmt.setFloat(5, temp_send);
                    }else{
                        pstmt.setNull(5,Types.FLOAT);}
                    if(humd_send != null) {
                        pstmt.setFloat(6, humd_send);
                    }else{
                        pstmt.setNull(6,Types.FLOAT);}
                    if(press_send != null) {
                        pstmt.setFloat(7, press_send);
                    }else{
                        pstmt.setNull(7,Types.FLOAT);
                    }
                    pstmt.execute();
                }
            } catch (Exception e){
                e.printStackTrace();
            }
        }
        private void newSensoresSQL(char DHT11, char BMP280){
            String url = "jdbc:mysql://localhost:3306/tabeladb";
            String user = "root";
            String password = "Sins@741";
            String sql = "select * from Gateways";
            String columnLabel = "";
            String putQuery = "INSERT INTO Gateways(ID,Sensor,Status) VALUES(?, ? , ?)";
            try{
                Class.forName("com.mysql.cj.jdbc.Driver");
            } catch(ClassNotFoundException e) {
                e.printStackTrace();
            }
            try {
                try(Connection con = DriverManager.getConnection(url, user, password); Statement statement = con.createStatement();) {
                    String sensor = " ";
                    int status = 1;
                    PreparedStatement pstmt = con.prepareStatement(putQuery);
                        for (int i = 0; i < 2; i++) {
                            if (DHT11 == '*' && i == 0) {
                                sensor = "DHT11";
                                status = 0;
                            }else if(DHT11=='1' && i == 0){
                                sensor = "DHT11";
                            }
                            if (BMP280 == '*' && i == 1) {
                                sensor = "BMP280";
                                status = 0;
                            }else if(BMP280 == '2' && i == 1){sensor = "BMP280";}

                            pstmt.setString(1, quem_sou);
                            pstmt.setString(2, sensor);
                            pstmt.setInt(3, status); //Status a 1 equivale a um sensor online
                            pstmt.execute();
                        }
                }catch (Exception e){
                    e.printStackTrace();
                }
            } catch (Exception e){
                e.printStackTrace();
            }
        }
        private void updateSensoresSQL(char qual,int status){ //Tenho de alterar a query feita, deve ser procurar aqueles que estão associados e a partir dai
            String url = "jdbc:mysql://localhost:3306/tabeladb";
            String user = "root";
            String password = "Sins@741";
            String sql = "select * from Gateways";
            String columnLabel = "";
            String putQuery = "UPDATE Gateways SET Status = ? WHERE ID = ? AND Sensor = ?";
            String what_to_send = " ";
            try{
                Class.forName("com.mysql.cj.jdbc.Driver");
            } catch(ClassNotFoundException e) {
                e.printStackTrace();
            }
            try {
                try(Connection con = DriverManager.getConnection(url, user, password); Statement statement = con.createStatement();) {
                    if (qual != '3') {
                        PreparedStatement pstmt = con.prepareStatement(putQuery);
                        if (qual == '1') {
                            what_to_send = "DHT11";
                        }
                        if (qual == '2') {
                            what_to_send = "BMP280";
                        }
                        //pstmt.setInt(1,i);
                        pstmt.setInt(1, status);
                        pstmt.setString(2, quem_sou);
                        pstmt.setString(3, what_to_send);
                        pstmt.execute();
                    }else{
                        putQuery = "UPDATE Gateways SET Status = ? WHERE ID = ?";
                        PreparedStatement pstmt = con.prepareStatement(putQuery);
                        pstmt.setInt(1, status);
                        pstmt.setString(2, quem_sou);
                        pstmt.execute();
                    }
                }
            } catch (Exception e){
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
        public void incrementEntrada(){
            this.entrada = entrada + 1;
        }
        public void setLocal(String latitude2, String Longitude2){
            this.latitude = latitude2;
            this.longitude=Longitude2;
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
           // System.out.println(authentication_array.length);
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
                    //System.out.println(array_char[i]);
                }
            }

            if(authentication_array[0] == Authenticationbyte){
                byte[] newArray = Arrays.copyOfRange(authentication_array, 1, 5);
                byte[] client_name = Arrays.copyOfRange(authentication_array, 5,12);
                byte[] latitude = Arrays.copyOfRange(authentication_array,12,19);
                byte[] longitude =  Arrays.copyOfRange(authentication_array,19,26);
                char[] password_to_match = Password_to_Match.toCharArray();
                byte[] password_byte = new byte[password_to_match.length];
                char[] client_name_char = new char[client_name.length];
                char[] client_latitude = new char[latitude.length];
                char[] client_longitude = new char[longitude.length];

                for(int i = 0 ; i<password_to_match.length; i++ ){
                    password_byte[i] = (byte)password_to_match[i];
                }
                for(int i = 0 ; i<client_name.length; i++){
                    client_name_char[i] = (char)client_name[i];
                }
                for(int i = 0 ; i<latitude.length; i++){
                    client_latitude[i] = (char)latitude[i];
                }
                for(int i = 0 ; i<longitude.length; i++){
                    client_longitude[i] = (char)longitude[i];
                }
                if(Arrays.equals(newArray, password_byte)){
                    System.out.println("Autenticado");
                    String tmp = new String(client_name_char);
                    String tmp2 = new String(client_latitude);
                    String tmp3 = new String(client_longitude);
                    setLocal(tmp2,tmp3);
                    authenticated = true;
                    setAuthenticated(authenticated);
                    setHostName(tmp);
                    System.out.println("Eu sou um novo cliente com o nome: " + tmp + " num local com a latitude: " + tmp2 + " e longitude: " + tmp3);
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
                                    System.out.println("START BYTE");
                                    unblocked = true; //Serve como mensagem de que vai começar a enviar mensagens
                                    char estado_DHT11 = ((char) arrayreceived[1]);
                                    char estado_BMP280 = ((char) arrayreceived[2]);
                                    newSensoresSQL(estado_DHT11,estado_BMP280);
                                    System.out.println("Cliente " + quem_sou + " está pronto para enviar dados" );
                                break;
                            case STOPbyte:
                                    System.out.println("Cliente " + quem_sou + " não vai enviar mais mensagens");
                                    char qual = (char) arrayreceived[1];
                                    int status = (int) arrayreceived[2];
                                    updateSensoresSQL(qual, status);
                                    setStatus(false);
                                break;
                            case ERRORbyte:
                                char[] time_received_error= {(char) arrayreceived[1], (char)arrayreceived[2],(char)arrayreceived[3],(char)arrayreceived[4],(char)arrayreceived[5],(char)arrayreceived[6],(char)arrayreceived[7],(char)arrayreceived[8]};
                                char error_type = (char)arrayreceived[9];
                                String time = String.valueOf(time_received_error);
                                System.out.println("Client: " + quem_sou + " teve um erro do tipo " + error_type + " pelas: " + time);
                                switch(error_type){
                                    /*case '1':
                                       byte[] send_error = {STOPbyte, (byte) '1'};
                                        toclient.write('2');
                                        toclient.flush();
                                        toclient.write(send_error);
                                        toclient.flush();
                                        break;
                                    case '2':
                                        byte[] send_error2 = {STOPbyte, (byte) '2'};
                                        toclient.write('2');
                                        toclient.flush();
                                        toclient.write(send_error2);
                                        toclient.flush();
                                        break;
                                    case '3':
                                        byte[] send_error3 = {STOPbyte, (byte) '3'};
                                        toclient.write('2');
                                        toclient.flush();
                                        toclient.write(send_error3);
                                        toclient.flush();
                                        break;
                                    case '4':
                                        byte[] send_error4 = {ENDbyte};
                                        toclient.write('1');
                                        toclient.flush();
                                        toclient.write(send_error4);
                                        toclient.flush();
                                        break;*/
                                    default:
                                        System.out.println("Tipo de Erro desconhecido");
                                        break;
                                }
                                break;
                            case DATAbyte:
                                csv_write = new CSVWriter(outputfile);
                                // Tipo - 1, Timestamp - 8, Dados - 17 bytes(5-TEMP,5-Humd,7- Press_atm)
                                char[] time_received= {(char) arrayreceived[1], (char)arrayreceived[2],(char)arrayreceived[3],(char)arrayreceived[4],(char)arrayreceived[5],(char)arrayreceived[6],(char)arrayreceived[7],(char)arrayreceived[8],(char)arrayreceived[9],(char)arrayreceived[10],(char)arrayreceived[11],(char)arrayreceived[12],(char)arrayreceived[13],(char)arrayreceived[14],(char)arrayreceived[15],(char)arrayreceived[16],(char)arrayreceived[17],(char)arrayreceived[18],(char)arrayreceived[19]};
                                char[] Temp = {(char)arrayreceived[20],(char)arrayreceived[21],(char)arrayreceived[22],(char)arrayreceived[23],(char)arrayreceived[24]};
                                char[] Humd = {(char)arrayreceived[25],(char)arrayreceived[26],(char)arrayreceived[27],(char)arrayreceived[28],(char)arrayreceived[29]};
                                char[] Press = {(char)arrayreceived[30],(char)arrayreceived[31],(char)arrayreceived[32],(char)arrayreceived[33],(char)arrayreceived[34],(char)arrayreceived[35],(char)arrayreceived[36]};
                                String time_stamp = String.valueOf(time_received);
                                String string_temp = String.valueOf(Temp);
                                System.out.println(string_temp);
                                String string_humd = String.valueOf(Humd);
                                System.out.println(string_humd);
                                String string_Press = String.valueOf(Press);
                                System.out.println(string_Press);
                                int i = 1;
                                updatesql(entrada,time_stamp,quem_sou,latitude,string_temp,string_humd,string_Press,longitude);
                                System.out.println(time_stamp);
                                incrementEntrada();
                                String[] data_input ={ time_stamp ,string_temp , string_humd , string_Press + "\n"};
                                csv_write.writeNext(data_input);
                                //Parte de processamento
                                csv_write.flush();
                                break;
                            default:
                                System.out.println(Arrays.toString(arrayreceived));
                                System.out.println(arrayreceived[0]);
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
                } catch(NumberFormatException e){
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