import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.InputStreamReader;
import java.io.OutputStreamWriter;
import java.net.Socket;

public class Test_client2 {

        public static void main(String[] args) throws Exception {
            Socket client_socket = new Socket("localhost", 4444);
            BufferedReader reader = new BufferedReader(new InputStreamReader(client_socket.getInputStream()));
            BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(client_socket.getOutputStream()));
            String serverMsg = null;

            if(client_socket.isConnected()){
                System.out.println("HELLO2");
            }
            while(true){
                if(client_socket.isClosed()){
                    return;
                }
                // System.out.println(client_socket.getLocalSocketAddress());
            }
        }
}
