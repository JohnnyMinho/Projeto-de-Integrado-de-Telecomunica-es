import java.io.IOException;
import java.net.ServerSocket;
import java.util.InputMismatchException;
import java.util.Scanner;

public class GestorServico {
    /* falta fazer, criador de threads, conexões tcp, envio de comandos e receção de confirmação,
    conexão a uma database, envio de dados e confirmação para uma data base (sqlite ou mysql), GUI
     */
    //TIPOS DE HEADER
    byte STOPbyte = 0b000000;
    byte STARTbyte = 0b0000000;
    byte BEGINbyte = 0b0000000;
    byte ENDbyte = 0b0000000;
    byte DATAbyte = 0b0000000;
    //---------------
    ServerSocket server;
    ClientWorker Novo_Cliente;
    String password = "TRY1";
    Thread [] Thread_array = new Thread[20];

    public static void main(String[] args) throws IOException {
        int port_to_be_used = 4444;
        int n_Clients = 0;
        int option;
        GestorServico gestorservico = new GestorServico();
        boolean Started = false;
        if(Started){
            Started = gestorservico.socketavailabilitychecker(port_to_be_used);
        }
        do{
        Scanner input  = new Scanner(System.in);
        System.out.println("***********GESTOR SERVIÇOS METEOROLOGIA****************");
        System.out.println("* Sistemas Sensores Conectados: "+n_Clients+"                      *");
        System.out.println("*          1-> Conectar novo Sistema Sensor           *"); //ESPERA 30 SEGUNDOS POR UMA NOVA CONEXÃO TCP AO SISTEMA CENTRAL, OTHERWISE PARA AO FIM DE 30 SEGUNDOS
        System.out.println("*          2-> Sistemas Sensores Conectados           *"); //MOSTRA O ID E NUMERO DE CLIENTE ASSOCIADA A UM SISTEMA SENSOR LIGADO
        System.out.println("*          3-> Ver Base de Dados                      *"); //MOSTRA O CONTEUDO DA BASE DE DADOS
        System.out.println("*          4-> Parar Todas as conexões                *"); //ENVIA UMA MENSAGEM DE STOP A TODOS OS CLIENTES TCP CONECTADOS
        System.out.println("*          5-> Renicializar todas as conexões         *"); //ENVIA UMA MENSAGEM DE BEGIN A TODOS OS CLIENTES TCP CONECTADOS
        System.out.println("*          6->                                        *");
        System.out.println("*          0-> ENCERRAR (KILL SYSTEM)                 *"); //NÃO USAR A NÃO SER PARA RAZÕES DE TESTE, ESTA OPÇÃO ENCERRA O SERVIDOR TCP
        System.out.println("*                                                     *");
        System.out.println("*               Made by G1_MIETI_PIT                  *");
        System.out.println("*******************************************************");
        try{
            option = input.nextInt();
        }catch(InputMismatchException e) {
            System.out.println("Invalid Option");
            input.nextLine();
            option = -1;
        }

            switch(option){
                case 1:
                    break;
                case 2:
                    break;
                case 3:
                    break;
                case 4:
                    break;
                case 5:
                    break;
            }
        }while(option != 0 && option != -1);
    }

    public boolean socketavailabilitychecker(int port_used) throws IOException {
        boolean sucess = false;
        try{
            server = new ServerSocket(port_used);
        }catch(IOException e){
            System.out.println("Não consigo ler a partir desta porta");
            System.out.println("Tente com outra porta (maior que 1024)");
        }finally{
            sucess = true;
        }
        return sucess;
    }
    public void searchforclients(ServerSocket server_on, int Numero_clientes) throws IOException{
        boolean status = false;
        while(!status) {
            try {
                ClientWorker novo_client = new ClientWorker(server_on.accept(), password);
                Thread_array[Numero_clientes] = new Thread(novo_client);
                Thread_array[Numero_clientes].start();
                status = true;
            } catch (IOException e) {
                System.out.println("FAILED TO CONNECT TO SERVER");
                Thread_array[Numero_clientes].interrupt();
                status = false;
            }
        }
    }
}
