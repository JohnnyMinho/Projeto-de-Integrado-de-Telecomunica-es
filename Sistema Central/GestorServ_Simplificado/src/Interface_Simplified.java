import javax.swing.*;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

public class Interface_Simplified {
    private JButton Begin;
    private JButton endButton;
    private JButton encerrarServidorButton;
    private JPanel Main_Panel;

    static Server new_server = new Server();
    static Thread new_thread = new Thread(new_server);
    public Interface_Simplified() {
        Begin.addActionListener(new Send_Begin());
        endButton.addActionListener(new Send_End());
        encerrarServidorButton.addActionListener(new Kill_Server());
    }

    private class Send_Begin implements ActionListener{
        @Override
        public void actionPerformed(ActionEvent e) {
            new_server.send_message(1);
        }
    }

    private class Send_End implements ActionListener{
        @Override
        public void actionPerformed(ActionEvent e) {
            new_server.send_message(2);
        }
    }

    private class Kill_Server implements ActionListener{
        @Override
        public void actionPerformed(ActionEvent e){
            new_server.setStatus(false);
        }
    }

    public static void main(String[] args) {
        JFrame frame = new JFrame("Interface_Simplified");
        frame.setContentPane(new Interface_Simplified().Main_Panel);
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
        frame.pack();
        frame.setVisible(true);
        new_thread.start();
    }
}
