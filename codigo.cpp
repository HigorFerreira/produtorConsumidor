#include<iostream>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include<vector>

using namespace std;

int Rand(){
	return rand() % 1024;
}

class Node{
	int *num;
	Node *prev;
	Node *next;
	
	void alocate(){
		this->num = new int;
		this->prev = 0;
		this->next = 0;
	}
public:
	Node(){
		alocate();
	}
	
	Node(int num){
		alocate();
		*this->num = num;
	}
	
	Node(int num, Node *prev){
		alocate();
		*this->num = num;
		this->prev = prev;
		this->prev->setNext(this);
	}
	
	int get(){
		return *this->num;
	}
	
	void set(int num){
		*this->num = num;
	}
	
	void setPrev(Node *prev){
		this->prev = prev;
	}
	
	void setNext(Node *next){
		this->next = next;
	}
	
	Node * getNext(){
		return this->next;
	}
	
	Node *getPrev(){
		return this->prev;
	}
};

class List{
	
	Node *base, *top;
	
	//Shared memory segment identifier
	int segment_id;
	//Shared memory pointer
	Node *buffer;
	
	vector<int> freeIndex;
	
	//Traffic light constants
	static const bool RED = 0;
	static const bool GREEN = 1;
	
	//TrafficLight
	bool trafficLight = RED;
	
	//Buffer size const	
	static const int BUFFER_SIZE = 0xA2C3;
	
	int bufferCounter = 0;
	
public:
	List(){
		this->base = this->top = 0;
		
		//Setting shared buffer in the memory
		segment_id = shmget(IPC_PRIVATE, BUFFER_SIZE*sizeof(Node), S_IRUSR | S_IWUSR);
		this->buffer = (Node *) shmat(segment_id, NULL, 0);
		
		//Node for set buffer initial values
		Node init;
		init.set(0);
		
		//Initializing buffer
		for(int i = 0; i < BUFFER_SIZE; i++)
			buffer[i] = init;
			
		//
		trafficLight = GREEN;
	}
	
	int insert(Node *node){
		if(bufferCounter > BUFFER_SIZE) return -1;

		//LOCKING OPERATIONS ON SHARED MEMORY
		trafficLight = RED;
		
		if(this->base == 0 && this->top == 0){
			buffer[bufferCounter] = *node;
			bufferCounter++;
		}
		else{

		}
		
		
		//UNLOCKING OPERATIONS ON SHARED MEMORY
		trafficLight = GREEN;
		return 0;
		delete node;
	}
	
	void test(){
		cout<<buffer[0].get()<<endl;
	}
	
};

int main(){
	List lista;
	
	Node *a = new Node(5);
	
	lista.insert(a);
	lista.test();
}