import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import java.text.DecimalFormat;
import java.text.DecimalFormatSymbols;
import java.time.LocalDateTime;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.Arrays;
import java.util.Date;
import java.util.Random;

public class Cliente_Simulado{

      static  long time_client = 0L;

    static public void setTime(long time2) {
        Cliente_Simulado.time_client = time2;
    }

    public static void main(String[] args) throws Exception {
            // Variáveis ligadas à ligação do socket e os stream associados
            Socket client_socket = new Socket("192.168.31.71", 4444); //Abertura de um socket ligado ao localhost através da porta 4444
            Date current_time = new Date(); //
            DataInputStream reader = (new DataInputStream(client_socket.getInputStream()));
            DataOutputStream writer = (new DataOutputStream(client_socket.getOutputStream()));
            String serverMsg = null;
            boolean stop = false;

            //temperatura default
            float temp = 0.00f;
            float max_temp_pretendido = 21.00f; //isto é de exemplo
            float min_temp_pretendido = 18.00f;
            float valorTemp = 0.1f; //valor a acrescentar ou decrescentar

            //humidade default
            float humidade = 0.00f;
            float max_hum_pretendido = 60.00f; //isto é de exemplo
            float min_hum_pretendido = 90.00f;
            float valorHum = 0.2f;

            //pressao default
            float pressao = 0.00f;
            float max_press_pretendido = 1000.0f; //isto é de exemplo
            float min_press_pretendido = 980.00f;
            float valorPress = 1.5f;

            temp = (float) (min_temp_pretendido + (Math.random() * ( max_temp_pretendido-  min_temp_pretendido)));
            System.out.println(temp);
            humidade = (float)  (min_hum_pretendido + (Math.random() * ( max_hum_pretendido-  min_hum_pretendido)));
            System.out.println(humidade);
            pressao = (float) (min_press_pretendido + (Math.random() * ( max_press_pretendido-  min_press_pretendido)));
            System.out.println(pressao);

            boolean send_authentication = true;
            boolean authenticated = false;
            boolean dead = false;

            String message = "TRY1HELLO2041.3083-8.0800";
            char[] char_array = message.toCharArray();
            byte[] byte_array = new byte[27];
            byte[] byte_array2 = new byte[37];

            long last_message_time = 0;
            Random Rd_Gen = new Random();

            for(int i = 0; i<15; i++) {
                byte_array[i + 1] = (byte) char_array[i];
            }
            byte_array2[0] = 0b00000101;
            byte_array[0] = 0b00001000;
            byte_array[24] = '\0';
            byte_array2[36]='\0';
            while(true) {
                int n = 0;
                if (send_authentication) {
                    send_authentication = false;
                    writer.write(Byte.parseByte(String.valueOf(byte_array.length)));
                    writer.write(byte_array);
                    writer.flush();
                    System.out.println(Arrays.toString(byte_array));
                }
                while (!stop) {
                    if (authenticated) {
                        if (ZonedDateTime.now().toInstant().toEpochMilli() - time_client > 1000) {
                            DecimalFormatSymbols otherSymbols = new DecimalFormatSymbols();
                            otherSymbols.setDecimalSeparator('.');
                            DecimalFormat formatador = new DecimalFormat("#.00",otherSymbols);
                            DateTimeFormatter dtf = DateTimeFormatter.ofPattern("dd/MM/yyyy HH:mm:ss");
                            LocalDateTime now = LocalDateTime.now();
                            String time = dtf.format(now);
                            String message_to_send = time + formatador.format(temp) + formatador.format(humidade) + formatador.format(pressao);
                            char[] char_array_to_send = message_to_send.toCharArray();
                            System.out.println("Mensagem Enviada: " + message_to_send);

                            byte[] byte_array_send = message_to_send.getBytes(StandardCharsets.UTF_8);
                            System.arraycopy(byte_array_send, 0, byte_array2, 1, byte_array_send.length);
                            writer.write(Byte.parseByte(String.valueOf(byte_array2.length)));
                            writer.flush();
                            writer.write(byte_array2);
                            writer.flush();
                            Boolean what_to_do = Rd_Gen.nextBoolean();
                            if (what_to_do && temp != max_temp_pretendido || what_to_do && humidade !=  max_hum_pretendido || what_to_do && pressao !=  max_press_pretendido ) {
                                temp += valorTemp;
                                humidade += valorHum;
                                pressao += valorPress;
                            } else if(!what_to_do && temp != max_temp_pretendido || !what_to_do && humidade !=  max_hum_pretendido || !what_to_do && pressao !=  max_press_pretendido ){
                                temp -= valorTemp;
                                humidade -= valorHum;
                                pressao -= valorPress;
                            }
                            System.out.println(Arrays.toString(byte_array2));
                            time_client = ZonedDateTime.now().toInstant().toEpochMilli();
                        }
                    }
                    if ((n = reader.available()) > 0) {
                        byte[] in_array = new byte[n];
                        reader.readFully(in_array, 0, n);
                        if (in_array[0] == 0b00000011) {
                            authenticated = true;
                        }
                        if (in_array[0] == 0b00000100) {
                            System.out.println("Server killed the connection");
                            dead = true;
                        }
                        for (int i = 0; i < n; i++) {
                            System.out.println(in_array[i]);
                        }
                    }
                    if (dead) {
                        writer.write(Byte.parseByte(String.valueOf(1)));
                        writer.write(0b00000001);
                        stop = true;
                        client_socket.close();
                    }
                    if (client_socket.isClosed()) {
                        return;
                    }
                    // System.out.println(client_socket.getLocalSocketAddress());
                }
            }
        }
}
