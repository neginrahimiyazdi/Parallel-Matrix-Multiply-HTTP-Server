//final upload
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include "/home/negin/Downloads/cpp-httplib-master (1)/cpp-httplib-master/httplib.h"
#include <semaphore.h>

//using namespace std;
using namespace httplib;

#define SHM_SIZE 1024
#define SEM_KEY 1234
#define SHM_KEY 8

std::mutex mtx;
sem_t semaphore;

void worker_process(int worker_id, int start, int end, int *shm_ptr, int row1, int column1, int row2, int column2) {


    int matrix1_size = row1 * column1;
    int start_of_matrix_c = matrix1_size + row2 * column2;


    //mtx.lock();
    int temp = 0;
    for (int i = start; i <end ; i++) {
        
        for (int j = 0; j < column2; j++) {
            mtx.lock();
            temp = 0;
            //shm_ptr[start_of_matrix_c + i * column2 + j]  = 0;
            for (int k = 0; k < column1; k++) {

                temp += shm_ptr[i * column1 + k] * shm_ptr[ matrix1_size + k * column2 + j];
                //or
                //shm_ptr[start_of_matrix_c + i * column2 + j] += shm_ptr[i * column1 + k] * shm_ptr[ matrix1_size + k * column2 + j];
            }
            shm_ptr[start_of_matrix_c + i * column2 + j] = temp;

            mtx.unlock();
            
           
        }
    }
    //mtx.unlock();
    
    sem_post(&semaphore);

}

std::string solve(int *shm_ptr, int row1, int column1, int row2, int column2, int num_workers){

    sem_init(&semaphore, 0, num_workers);
    int chunk_size = row1 / num_workers;
    int remain = row1 % num_workers;
    int x_remain = 0;
    int start = 0;
    int value;
    int end = 0;
    std::vector<std::thread> threads;
    int t = row1*column1+row2*column2;
 
    for (int i = 0 ; i < num_workers; i++) {
        
        start = end;
        end += chunk_size;
        if(remain){
            remain-=1;
            end+=1;
        }
        sem_wait(&semaphore);
        threads.emplace_back(worker_process, i, start, end, shm_ptr, row1, column1, row2, column2);
    }
       
        

   /* 
    for (auto& thread : threads) {
           thread.join();
    }
    */



    while(value != num_workers){
        sem_getvalue(&semaphore, &value);
    }
    
    for (auto& thread : threads) {
           thread.detach();
    }
    

    t =  row1* column1 + row2* column2;
    int result_matrix_ele = row1 * column2 + t;

    
    int l = t;
    std::string solution= "result:\n";
    for (int i = 0; i < row1; i++) {
        for (int j = 0; j < column2; j++) {
            //cout<<shm_ptr[l]<<" ";
            solution += std::to_string(shm_ptr[l++]) + " ";
            
        }
            //cout<<endl;
            solution+='\n';
    }

    //cout<<"here solution"<<solution<<endl;
    return solution;
}



int main() {
    
    
    Server svr;

    int row1,row2,column2,column1;
    int size = 100;
    int matrix2[100][100] , matrix1[100][100] , result[100][100];
    
    
    int sem_id;
    key_t key = ftok("shmfile",65);
    int shm_id = shmget(IPC_PRIVATE, size * sizeof(int), IPC_CREAT | 0666);
    int *shm_ptr = (int*) shmat(shm_id, NULL, 0);

    int num_workers = 2;

    

    

    svr.Post("/matrix1_matrix2", [&](const Request& req, Response& res) {
        //a[0] = 0;
        std::string matrix_str = req.body;
        std::stringstream ss(matrix_str);
        std::string line;
        int row = 0;
        int matrixIndex = 1;
        int j = 0, k =0;

        while (std::getline(ss, line, '_')) {
            std::stringstream line_ss(line);
            std::string token;
            int col = 0;

            while (std::getline(line_ss, token, '|')) {
                std::stringstream token_ss(token);
                std::string element;

                while (std::getline(token_ss, element, ',')) {
                    if (matrixIndex == 1) {
                        matrix1[row][j] = std::stoi(element);
                        
                        j+=1;
                        column1 = j;

                    } else {
                        matrix2[row][k] = std::stoi(element);
                        k+=1;
                        column2 = k;

                    }
                }
                j = 0;
                k = 0; 
                row++;
            }
            if (matrixIndex == 1) {
                row1 = row;
                row = 0;
                matrixIndex = 2;
            } else {
                row2 = row;
                break; 
            }
        }
        int t = 0;
        std::cout<<"Matrix1 : "<<std::endl;
        for(int l =0 ;l <row1 ; l++){
            for(int f =0; f< column1; f++){
                std::cout<< matrix1[l][f]<< " ";
                shm_ptr[l*column1 + f] =  matrix1[l][f];
                //cout<< shm_ptr[l][f]<< " ";
                t++;

            }
            std::cout<<std::endl;
        }
        int c = row1*column1;
        std::cout<<"Matrix2 : "<<std::endl;
        for(int l =0 ;l <row2 ; l++){
            for(int f =0; f< column2; f++){
                std::cout<< matrix2[l][f]<< " ";
                shm_ptr[c+ l*column2 + f] =  matrix2[l][f];
                //cout<< shm_ptr[l][f]<< " ";
                t++;
            }   
            std::cout<<std::endl;
        }
        std::cout<<"in shared list:"<<std::endl;
        for(int l =0 ;l < t ; l++){
            std::cout<< shm_ptr[l]<< " ";
        }   
        std::cout<<std::endl;

        std::cout<<"column1 : "<<column1<<std::endl;
        std::cout<<"row1 : "<<row1<<std::endl;
        std::cout<<"column2 : "<<column2<<std::endl;
        std::cout<<"row2 : "<<row2<<std::endl;
        std::cout << "Received two matrix\n";

        res.set_content("TWO Matrix received", "text/plain");
    });
    
    svr.Post("/num_workers", [&](const Request& req, Response& res) {
        
        std::string number = req.body;
        num_workers =  stoi(number);
        std::cout<<"num_workers: "<<num_workers<<std::endl;
        
        res.set_content("Number of workers received", "text/plain");

    });

    svr.Get("/result", [&](const Request& req, Response& res)   {
        
        std::string solution = "";
        std::string number = req.body;
        solution = solve(shm_ptr,row1,column1,row2,column2,num_workers);    
        
        //std::cout << "Some text" << std::endl << std::flush;

        std::cout<<solution<<std::endl;
        res.set_content(solution, "text/plain");
    });
    
    svr.Get("/stop", [&](const Request& req, Response& res) {
        shmdt(shm_ptr);
        shmctl(shm_id, IPC_RMID, NULL);
        svr.stop();
    });

    std::cout << "Server running on port 8080" << std::endl;
    svr.listen("localhost", 8080);

    return 0;

}



/*

    svr.Post("/matrix1_matrix2", [&matrix1, &matrix2](const Request& req, Response& res) {
        //a[0] = 0;
        string matrix_str = req.body;
        std::stringstream ss(matrix_str);
        std::string line;
        int row = 0;
        int matrixIndex = 1;
        int j = 0, k =0;

        while (std::getline(ss, line, '-')) {
            std::stringstream line_ss(line);
            std::string token;
            int col = 0;

            while (std::getline(line_ss, token, '|')) {
                std::stringstream token_ss(token);
                std::string element;

                while (std::getline(token_ss, element, ',')) {
                    if (matrixIndex == 1) {
                        matrix1[row][j] = std::stoi(element);
                        *shm_ptr = 
                        j+=1;
                        column1 = j;

                    } else {
                        matrix2[row][k] = std::stoi(element);
                        k+=1;
                        column2 = k;

                    }
                }
                j = 0;
                k = 0; 
                row++;
            }
            if (matrixIndex == 1) {
                row1 = row;
                row = 0;
                matrixIndex = 2;
            } else {
                row2 = row;
                break; 
            }
        }
        cout<<"Matrix1 : "<<endl;
        for(int l =0 ;l <row1 ; l++){
            for(int f =0; f< column1; f++){
                cout<< matrix1[l][f]<< " ";
            }
            cout<<endl;
        }

        cout<<"Matrix2 : "<<endl;
        for(int l =0 ;l <row2 ; l++){
            for(int f =0; f< column2; f++){
                cout<< matrix2[l][f]<< " ";
            }
            cout<<endl;
        }

        std::cout<<"column1 : "<<column1<<endl;
        std::cout<<"row1 : "<<row1<<endl;
        std::cout<<"column2 : "<<column2<<endl;
        std::cout<<"row2 : "<<row2<<endl;
        std::cout << "Received two matrix\n";

        res.set_content("TWO Matrix received", "text/plain");
    });
    
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <cstdlib>
#include <ctime>
#include <mutex>
#include "/home/negin/Downloads/cpp-httplib-master (1)/cpp-httplib-master/httplib.h"
#include <semaphore.h>

using namespace std;
using namespace httplib;


#define SHM_SIZE 1024
#define SEM_KEY 1234
#define SHM_KEY 8

int row1,row2,column2,column1;
int num_workers;
int sem_id, shm_id;
int *shm_ptr;
mutex mtx;
int matrix2[100][100] , matrix1[100][100] , result[100][100];
sem_t semaphore;


void worker_process(int worker_id, int start, int end) {

    
    int *matrix_a = new int[row1 * column1];
    int *matrix_b = new int[row2 * column2];
    int *matrix_c = new int[row1 * column2];
    int *shm_matrix_a = shm_ptr ;
    int *shm_matrix_b = shm_ptr + row1 * column1;
    int *shm_matrix_c = shm_ptr + row1 * column1 + row2 * column2 + column2* start* worker_id;
    int matrix1_size = row1 * column1;
    int start_of_matrix_c = matrix1_size + row2 * column2;


    cout<< "start: " << start<<endl;
    cout<< "end: " << end <<endl;
    // Copy matrix C to shared memory
    mtx.lock();

    cout<<"here solution of matrix A"<<endl;
    for(int i =0 ; i<4;i++){
        cout<<"shm_ptr: "<<shm_ptr[i]<<endl;
        cout<<"shm_matrix_a: "<<shm_matrix_a[i]<<endl;
    }

    cout<<"+++++++++++++++++"<<endl;
    cout<<"+++++++++++++++++"<<endl;

    cout<<"here solution of matrix B"<<endl;
    for(int i =0 ; i<4;i++){
        cout<<"shm_ptr: "<<shm_ptr[i + matrix1_size]<<endl;
        cout<<"shm_matrix_b: "<<shm_matrix_b[i]<<endl;
    }

    cout<<"+++++++++++++++++"<<endl;

    for (int i = start; i <end ; i++) {
        for (int j = 0; j < column2; j++) {
            shm_matrix_c[i * row1 + j] = 0;
            for (int k = 0; k < column1; k++) {
                //inja monde
                
                shm_matrix_c[i * row1 + j] += shm_matrix_a[i * row1 + k] * shm_matrix_b[k * row2 + j];
                
                //shm_matrix_c[i * row1 + j] += matrix_a[i * row1 + k] * matrix_b[k * column2 + j];
            }
            shm_ptr[start_of_matrix_c + i * row1 + j] = shm_matrix_c[i * row1 + j] ;
            cout<< "solution of dot"<< shm_matrix_c[i * row1 + j] << endl;
        }
    }

    mtx.unlock();
    
    delete[] matrix_a;
    delete[] matrix_b;
    delete[] matrix_c;
    sem_post(&semaphore);

}

string solve(){


    cout<<"here solve\n";

    int t = 0;
    for(int i = 0; i<row1; i++ ){
            for(int j =0; j<column1; j++){
                shm_ptr[t++] = matrix1[i][j];
            }
        }
    cout<<"here checl shm_matrix_a " << shm_ptr[0] << " sol" <<endl;
    cout<<"here checl shm_matrix_a " << shm_ptr[1] << " sol" <<endl;
    cout<<"here checl shm_matrix_a " << shm_ptr[2] << " sol" <<endl;
    cout<<"here checl shm_matrix_a " << shm_ptr[3] << " sol" <<endl;
    
    int number_of_matrix1 = t;
    for(int i = 0; i<row2; i++ ){
            for(int j =0; j<column2; j++){
                shm_ptr[t++] = matrix2[i][j];
        }
    }
    cout<<"++++++++++++++++++"<<endl;
    cout<<"here checl shm_matrix_b " << shm_ptr[4] << " sol" <<endl;
    cout<<"here checl shm_matrix_b " << shm_ptr[5] << " sol" <<endl;
    cout<<"here checl shm_matrix_b " << shm_ptr[6] << " sol" <<endl;
    cout<<"here checl shm_matrix_b " << shm_ptr[7] << " sol" <<endl;

    int chunk_size = row1 / num_workers;
    cout<<"number of row1:" <<row1 << endl;
    int start = 0;
    int end = chunk_size;


    for (int i = 0 ; i < num_workers; i++) {
        sem_wait(&semaphore);
        if(i == num_workers -1){
            start = end; 
            end = row1;
            worker_process(i,start,end);
        }else if(i==0){
            start = 0;
            end = chunk_size;
            worker_process(i,start,end);
        }else{
            start = end;
            end += chunk_size;
            worker_process(i,start,end); 
        }

    }


    cout << "Matrix C:" << endl;
    t =  row1* column1 + row2* column2;
    for (int i = 0; i < row1; i++) {
        for (int j = 0; j < column2; j++) {
            result[i][j] = shm_ptr[t];
            cout << shm_ptr[t++] << " ";
        
        }
        
        cout << endl;
    }
    

   
    
    string solution;

    cout<<"here";
        for (int i = 0; i < row1; i++) {
            for (int j = 0; j < column2; j++) {
                solution += result[i][j] + " ";
        }
            solution+='\n';
    }
    return solution;
     // Detach shared memory
    shmdt(shm_ptr);

    shmctl(shm_id, IPC_RMID, NULL);
}



int main() {
    
    num_workers = 2;
    Server svr;
    sem_init(&semaphore, 0, 1);
    key_t key = ftok("shmfile",65);
    int size = 100;

    shm_id = shmget(IPC_PRIVATE, size * sizeof(int), IPC_CREAT | 0666);

    // attach the shared memory segment to the program's address space
    shm_ptr = (int*) shmat(shm_id, NULL, 0);
    // Set up an endpoint for receiving the matrix
    svr.Post("/matrix1", [](const Request& req, Response& res) {
        // Parse the matrix from the request body
        string matrix_str1 = req.body;
        stringstream ss(matrix_str1);
        string row;
        
        int i=0,j=0;
        while (getline(ss, row, '|')) {
            stringstream ss_row(row);
            string item;

            while (getline(ss_row, item, ',')) {
                cout << item << " ";
                matrix1[i][j++] = stoi(item);
            }
            i++;
            column1 = j;
            j = 0;
            cout << "\n";
        }
        row1 = i;

        // Print the received matrix
        cout << "Received first matrix:\n";
        // Send a response indicating success
        res.set_content("First Matrix received", "text/plain");
    });
        svr.Post("/matrix1-matrix2", [](const Request& req, Response& res) {
        // Parse the matrix from the request body
        string matrix_str1 = req.body;
        stringstream ss(matrix_str1);
        string row;
        
        int i=0,j=0;
        while (getline(ss, row, '|')) {
            stringstream ss_row(row);
            string item;

            while (getline(ss_row, item, ',')) {
                cout << item << " ";
                matrix1[i][j++] = stoi(item);
            }
            i++;
            column1 = j;
            j = 0;
            cout << "\n";
        }
        row1 = i;

        // Print the received matrix
        cout << "Received first matrix:\n";
        // Send a response indicating success
        res.set_content("First Matrix received", "text/plain");
    });


    // Set up an endpoint for receiving the matrix
    svr.Post("/matrix2", [](const Request& req, Response& res) {
        // Parse the matrix from the request body
        string matrix_str2 = req.body;
        stringstream ss(matrix_str2);
        string row;
        
        int i=0,j=0;
        
        while (getline(ss, row, '|')) {
            stringstream ss_row(row);
            string item;
            //cout<< "here";
            while (getline(ss_row, item, ',')) {
                    cout << item << " ";
                    matrix2[i][j] = stoi(item);
                    j+=1;  
            }
            i++;
            column2 = j;
            j = 0;
            cout<<endl;
        }
        row2 = i;
        
        // Print the received matrix
        cout << "Received second matrix\n";
        cout<<"num of row2:"<< row2 << endl;
        // Send a response indicating success
        res.set_content("Second Matrix received", "text/plain");
    });

    svr.Post("/num_workers", [](const Request& req, Response& res) {
        // Parse the matrix from the request body
        string number = req.body;
        
        num_workers =  stoi(number);
        // Send a response indicating success
        cout<<num_workers<<endl;
        res.set_content("nummer of workers", "text/plain");
    });
    
        svr.Get("/result", [](const Request& req, Response& res) {
        // Parse the matrix from the request body
        string number = req.body;
        cout<<"here result\n";
        string solution = solve();
        
        cout<<solution;
        res.set_content(solution, "text/plain");
        //res.set_content(solution, "text/plain");
    });

    cout << "Server running on port 8080" << endl;
    svr.listen("localhost", 8080);
    
    //sem_destroy(&semaphore);
    return 0;

}

*/