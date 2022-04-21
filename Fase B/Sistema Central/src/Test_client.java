import java.io.*;
import java.net.Socket;
import java.util.Arrays;

public class Test_client {
    public static void main(String[] args) throws Exception {
        Socket client_socket = new Socket("localhost", 4444);
        DataInputStream reader = (new DataInputStream(client_socket.getInputStream()));
        DataOutputStream writer = (new DataOutputStream(client_socket.getOutputStream()));
        String serverMsg = null;
        boolean test = true;
        String message = "TRY1HELLO20";
        char[] char_array = message.toCharArray();
        byte[] byte_array = new byte[13];
        for(int i = 0; i<11; i++){
            byte_array[i+1] = (byte) char_array[i];
        }
        byte_array[0] = 0b00001000;
        byte_array[12] = '\0';
        while(true){
                Thread.sleep(10000);
                if(test) {
                    System.out.println("HELLO");
                    test = false;
                    writer.writeInt(byte_array.length);
                    writer.write(byte_array);
                    System.out.println(Arrays.toString(byte_array));
                }
            if(client_socket.isClosed()){
                return;
            }
           // System.out.println(client_socket.getLocalSocketAddress());
        }
    }
}
