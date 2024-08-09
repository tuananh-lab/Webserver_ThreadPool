#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <time.h>
#include <filesystem>

namespace fs = std::filesystem;
using namespace std;

int count = 1;
const int NUM_THREAD_POOLS = 4;
const string RECEIVED_FILES_DIR = "/home/tuananh/Downloads/OS/test"; 

struct HttpRequestInfo {
    int client_socket;
    string request_data;
};

struct threadpool_struct {
    vector<pthread_t> threads;
    pthread_mutex_t qlock;
    queue<HttpRequestInfo> request_queue;
};

typedef struct threadpool_struct* threadpool;

void* handle_http_request(void* arg);

threadpool create_threadpool(int num_threads_in_pool);

void destroy_threadpool(threadpool pool);

void run_http_server(threadpool pool);

int main() {
    if (mkdir(RECEIVED_FILES_DIR.c_str(), 0777) == -1 && errno != EEXIST) {
        perror("Failed to create directory");
        exit(EXIT_FAILURE);
    }

    threadpool pool = create_threadpool(NUM_THREAD_POOLS);

    if (pool == NULL) {
        fprintf(stderr, "Failed to create thread pool\n");
        exit(EXIT_FAILURE);
    }

    run_http_server(pool);

    destroy_threadpool(pool);

    return 0;
}

void send_response(int client_socket, const string& response) {
    send(client_socket, response.c_str(), response.size(), 0);
    close(client_socket);
}


std::vector<std::string> get_file_list(const std::string& directory) {
    std::vector<std::string> file_list;

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file()) {
            file_list.push_back(entry.path().filename().string());
        }
    }

    return file_list;
}

// Function to handle request GET "/file-list"
void handle_file_list_request(int client_socket) {
    std::vector<std::string> file_list = get_file_list(RECEIVED_FILES_DIR);

    // Convert the file names to a JSON array format
    std::string json_response = "[";
    for (const auto& file_name : file_list) {
        json_response += "\"" + file_name + "\",";
    }
    // Remove the trailing comma if there are files
    if (!file_list.empty()) {
        json_response.pop_back();
    }
    json_response += "]";

    // Transfer JSON respone to client
    std::string response = "HTTP/1.1 200 OK\r\nContent-Length: " + std::to_string(json_response.size()) + "\r\n\r\n" + json_response;
    send_response(client_socket, response);
}

// Function to handle manual request GET 
void handle_get_request(int client_socket, const std::string& path) {
    string file_path;
    string content_type;

    if (path == "/") {
        // If path is '/', using file index.html
        file_path = "/home/tuananh/Downloads/OS/index.html";
        content_type = "text/html; charset=utf-8";
    } else {
        // Otherwise, using path is file name
        string file_name = path.substr(1); // Remove first'/'
        file_path = RECEIVED_FILES_DIR + "/" + file_name;

        // Determine the Content-Type based on the file extension
        if (file_name.rfind(".html") != string::npos) {
            content_type = "text/html; charset=utf-8";
        } if (file_name.rfind(".css") != string::npos) {
            content_type = "text/css";
        } else if (file_name.rfind(".js") != string::npos) {
            content_type = "application/javascript";
        } else if (file_name.rfind(".jpg") != string::npos) {
            content_type = "image/jpeg";
        } else if (file_name.rfind(".txt") != string::npos) {
            content_type = "text/plain";
        } else if (file_name.rfind(".cpp") != string::npos) {
            content_type = "text/plain";
        }
    }
    
    

    // Open file to read content
    ifstream file(file_path, ios::binary);
    if (!file) {
        perror("Failed to open file");
        close(client_socket);
        return;
    }

    // Read file content
    stringstream ss;
    ss << file.rdbuf();
    string file_content = ss.str();

    // Send  HTTP response with file content to client
    string response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: " + to_string(file_content.size()) + "\r\n\r\n" + file_content;
    /*string response = "HTTP/1.1 200 OK\r\nContent-Type: image/jpeg\r\nContent-Length: " + to_string(file_content.size()) + "\r\n\r\n" + file_content;*/
    send_response(client_socket, response);
}


void* handle_http_request(void* arg) {
    threadpool pool = (threadpool)arg;

    while (true) {
        HttpRequestInfo request_info;

        pthread_mutex_lock(&(pool->qlock));

        // Check if there are requests in the queue
        while (pool->request_queue.empty()) {
            // Release the lock and wait for a signal
            pthread_mutex_unlock(&(pool->qlock));
            usleep(100);
            pthread_mutex_lock(&(pool->qlock));
        }

        // Get the request from the queue
        request_info = pool->request_queue.front();
        pool->request_queue.pop();

        pthread_mutex_unlock(&(pool->qlock));

        // Process the request using the client_socket in request_info
        int client_socket = request_info.client_socket;
        const string& request_data = request_info.request_data.c_str();

        // Logging for debugging
        cout << "Received request: " << request_data << " Thread_id:" << pthread_self() << endl;

        // Parse HTTP request
        istringstream request(request_data);
        string method, path, version;
        request >> method >> path >> version;

// Handle POST requests
if (method == "POST") {
    // File name based on HTTP path
    string file_name;
    size_t filename_start = request_data.find("filename=\"");
    if (filename_start != string::npos) {
        filename_start += 10; // Move past "filename=\""
        size_t filename_end = request_data.find("\"", filename_start);
        if (filename_end != string::npos) {
            file_name = request_data.substr(filename_start, filename_end - filename_start);
        }
    }

    // Construct the full file path by combining the directory and the file name
    string file_path = RECEIVED_FILES_DIR + "/" + file_name;

    // Check if the file already exists
    int suffix = 1;
    while (fs::exists(file_path)) {
        // File already exists, remove old suffix if any, then add a new numerical suffix
        string extension = fs::path(file_name).extension().string();
        string base_name = fs::path(file_name).stem().string();

        // Remove old numerical suffix (if any)
        size_t hyphen_pos = base_name.find_last_of('-');
        if (hyphen_pos != string::npos) {
            base_name = base_name.substr(0, hyphen_pos);
        }

        // Construct the new file name with the numerical suffix
        file_name = base_name + "-" + to_string(suffix) + extension;

        // Construct the full file path with the updated file name
        file_path = RECEIVED_FILES_DIR + "/" + file_name;

        // Increment the suffix for the next iteration if needed
        suffix++;
    }

    // Open file to write content received from the client
    ofstream file(file_path, ios::binary);
    if (!file) {
        perror("Failed to open file");
        close(client_socket);
        continue;
    }

    // Write the content received from the client to the file
    file << request_data;
    file.close();

    // Send HTTP response to the client
    string response = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(file_name.size()) + "\r\n\r\n" + file_name;
    send_response(client_socket, response);
}


// Handle GET requests
else if (method == "GET") {
    if (path == "/file-list") {
                // Xử lý yêu cầu để lấy danh sách file
                handle_file_list_request(client_socket);
            } else {
                // Xử lý các yêu cầu GET thông thường
                handle_get_request(client_socket, path);
            }
}

         else {
            // Unsupported HTTP method, return 405 Method Not Allowed
            string response = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            send_response(client_socket, response);
        }
    }

    pthread_exit(NULL);
}

threadpool create_threadpool(int num_threads_in_pool) {
    if ((num_threads_in_pool <= 0) || (num_threads_in_pool > NUM_THREAD_POOLS))
        return NULL;

    threadpool pool = new struct threadpool_struct;

    if (pool == NULL) {
        fprintf(stderr, "Out of Memory !!!\n");
        return NULL;
    }

    pool->threads.resize(num_threads_in_pool);

    if (pthread_mutex_init(&pool->qlock, NULL) != 0) {
        fprintf(stderr, "Mutex initialization failed\n");
        delete pool;
        return NULL;
    }

    for (int i = 0; i < num_threads_in_pool; ++i) {
        if (pthread_create(&(pool->threads[i]), NULL, handle_http_request, (void*)pool) != 0) {
            fprintf(stderr, "Failed to create thread\n");
            exit(EXIT_FAILURE);
        }
    }

    return pool;
}

void destroy_threadpool(threadpool pool) {
    pthread_mutex_destroy(&(pool->qlock));
    delete pool;
}

void run_http_server(threadpool pool) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = inet_addr("192.168.1.48");
    server_address.sin_port = htons(3000);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, 5) == -1) {
        perror("Listening failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    cout << "HTTP Server is listening on port 3000..." << endl;

    while (true) {
        sockaddr_in client_address;
        socklen_t client_address_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_len);
        if (client_socket == -1) {
            perror("Acceptance failed");
            continue;
        }

        cout << "Connection from " << inet_ntoa(client_address.sin_addr) << endl;

        // Add the request to the queue
        HttpRequestInfo request_info;
        request_info.client_socket = client_socket;

        char buffer[4096];
        ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received <= 0) {
            close(client_socket);
            continue;
        }

        buffer[bytes_received] = '\0';
        request_info.request_data = buffer;

        pthread_mutex_lock(&(pool->qlock));
        pool->request_queue.push(request_info);
        pthread_mutex_unlock(&(pool->qlock));
    }
}
