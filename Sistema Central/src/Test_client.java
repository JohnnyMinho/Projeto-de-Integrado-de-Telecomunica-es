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
        boolean test2 = true;
        String message = "TRY1HELLO204600";
        String message2 = "10:10:1010.0011.001000.00";
        char[] char_array2 = message2.toCharArray();
        char[] char_array = message.toCharArray();
        byte[] byte_array = new byte[18];
        byte[] byte_array2 = new byte[27];
        for(int i = 0; i<15; i++){
            byte_array[i+1] = (byte) char_array[i];
        }
        for(int i = 0; i < 25; i++){
            byte_array2[i+1] = (byte) char_array2[i];
        }
        byte_array2[0] = 0b0000101;
        byte_array[0] = 0b00001000;
        byte_array[17] = '\0';
        byte_array2[26]='\0';
        while(true){
                int n = 0;
                if(test) {
                    System.out.println("HELLO");
                    test = false;
                    writer.writeInt(byte_array.length);
                    writer.write(byte_array);
                    writer.flush();
                    System.out.println(Arrays.toString(byte_array));
                }
                if(test2){
                    System.out.println("Hello2");
                    test2 = false;
                    writer.writeInt(byte_array2.length);
                    writer.write(byte_array2);
                    writer.flush();
                    System.out.println(Arrays.toString(byte_array2));
                }
                if((n=reader.available())>0){
                    byte[] in_array = new byte[n];
                    reader.readFully(in_array,0,n);
                    for(int i = 0; i<n;i++){
                        System.out.println(in_array[i]);
                    }
                }
            if(client_socket.isClosed()){
                return;
            }
           // System.out.println(client_socket.getLocalSocketAddress());
        }
    }
}
