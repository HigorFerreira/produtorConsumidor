// Aplicação cliente servidor e memoria compartilhada.
// Higor Ferreira 2019

#include<iostream>
#include<stdlib.h>
#include<sys/shm.h>
#include<sys/stat.h>
#include<vector>
#include<unistd.h>

using namespace std;

int maxRand = 30;

int Rand(){
	return rand() % maxRand + 1;
}

int Srand(int i){
	srand(i);
	return rand() % maxRand + 1;
}

class Node{
	int *num;
	int *index;
	Node *prev;
	Node *next;
	
	void alocate(){
		this->num = new int;
		this->index = new int;
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

	void setindex(int index){
		*this->index = index;
	}

	int getindex(){
		return *this->index;
	}
};

class List{
	
	Node *base, *top;
	
	//Shared memory pointer
	Node *buffer;
	
	vector<int> freeIndex;
	
	//Buffer size const	
	static const int BUFFER_SIZE = 0xA2C3;
	
	int bufferCounter = 0;
	
public:
	//Shared memory segment identifier
	int segment_id;

	//Traffic light constants
	static const bool RED = 0;
	static const bool GREEN = 1;
	
	//TrafficLight
	bool trafficLight = RED;

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
		// trafficLight = RED;
		
		if(!(this->base == 0 && this->top == 0)){
			Node *temp = base;
			while((node->get() > temp->get()) && temp != top){
				temp = temp->getNext();
			}
			
			Node *prev = new Node;
			if(temp != 0) prev = temp->getPrev();
			// Node *next = temp->getNext();

			
			if(temp == 0){
				this->top->setNext(node);
				node->setPrev(top);
				this->top = node;
			}
			else if(prev == 0){
				temp->setPrev(node);
				node->setNext(temp);
				this->base = node;
			}
			else{
				node->setNext(temp);
				node->setPrev(prev);

				prev->setNext(node);
				temp->setPrev(node);
			}
		}

		//Buffer insertion
		if(freeIndex.empty()){
			node->setindex(bufferCounter);
			buffer[bufferCounter] = *node;
			bufferCounter++;
		}
		else{
			node->setindex(freeIndex.at(0));
			buffer[freeIndex.at(0)] = *node;
			freeIndex.erase(freeIndex.begin());
		}
		
		delete node;
		//UNLOCKING OPERATIONS ON SHARED MEMORY
		// trafficLight = GREEN;
		return 0;
	}

	void remove(int index){

		//LOCKING OPERATIONS ON SHARED MEMORY
		// trafficLight = RED;

		Node *node = &buffer[index];
		//Extract the index position where this node are living
		int nodeIndex = node->getindex();

		if(node == this->top){
			this->top = node->getPrev();
			node->getPrev()->setNext(0);
			node->setPrev(0);
		}
		else if(node == this->base){
			this->base = node->getNext();
			this->base->setPrev(0);
			node->setNext(0);
		}
		else{
			Node *next = node->getNext();
			Node *prev = node->getPrev();

			prev->setNext(next);
			next->setPrev(prev);

			node->setNext(0);
			node->setPrev(0);
		}

		//Setting index as diponible
		freeIndex.push_back(nodeIndex);

		//UNLOCKING OPERATIONS ON SHARED MEMORY
		// trafficLight = GREEN;
	}

	int find(int num){

		Node *node = this->base;
		while(node != 0){
			if(node->get() == num){
				return node->getindex();
			}

			node = node->getNext();
		}

		return -1;
	}

	int getNodeValue(int index){
		Node *pt = &buffer[index];
		return pt->get();
	}
	
	void test(){
		cout<<buffer[0].get()<<endl;
	}
	
};

int main(){
	List list;
	
	int pid = fork();
	if(pid < 0){
		cout<<endl<<"Erro na criacao de processos filhos"<<endl<<endl;
	}
	else if(pid > 0){
		Node *buffer = (Node *) shmat(list.segment_id, NULL, 0);
		while(true){
			int *randNum = new int;
			*randNum = Srand(pid);

			cout<<endl<<"Searching by: "<<*randNum<<endl;
			int nodeFounded = list.find(*randNum);

			if(nodeFounded > -1){
				cout<<*randNum<<" Was founded"<<endl;
				while(true){
					if(list.trafficLight == List::GREEN){
						
						list.trafficLight = List::RED;

						int valueToInsert = 2*list.getNodeValue(nodeFounded);
						list.remove(nodeFounded);
						
						cout<<"Reinserting "<<valueToInsert<<endl;
						Node *node = new Node(valueToInsert);
						list.insert(node);
						cout<<valueToInsert<<" Reinserted"<<endl;
						cout<<"============================\n\n";

						list.trafficLight = List::GREEN;
					}
				}
			}

			for(long i = 0; i < 0xffffffff; i++);
		}
	}
	else{
		while(true){
			int *randNum = new int;
			*randNum = Rand();
			Node *node = new Node(Rand());
			if(!(list.insert(node) < 0)){
				cout<<*randNum<<" foi adicionado a lista\n";
				delete randNum;
			}
			else{
				cout<<"Memoria insuficiente\n";
			}
			for(long i = 0; i < 0x28ffffff; i++);
		}
	}
}
