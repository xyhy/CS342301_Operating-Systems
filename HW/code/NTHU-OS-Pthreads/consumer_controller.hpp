#include <pthread.h>
#include <unistd.h>
#include <vector>
#include <iostream>
#include "consumer.hpp"
#include "ts_queue.hpp"
#include "item.hpp"
#include "transformer.hpp"

#ifndef CONSUMER_CONTROLLER
#define CONSUMER_CONTROLLER

class ConsumerController : public Thread {
public:
	// constructor
	ConsumerController(
		TSQueue<Item*>* worker_queue,
		TSQueue<Item*>* writer_queue,
		Transformer* transformer,
		int check_period,
		int low_threshold,
		int high_threshold
	);

	// destructor
	~ConsumerController();

	virtual void start();

private:
	std::vector<Consumer*> consumers;

	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	Transformer* transformer;

	// Check to scale down or scale up every check period in microseconds.
	int check_period;
	// When the number of items in the worker queue is lower than low_threshold,
	// the number of consumers scaled down by 1.
	int low_threshold;
	// When the number of items in the worker queue is higher than high_threshold,
	// the number of consumers scaled up by 1.
	int high_threshold;

	static void* process(void* arg);
};

// Implementation start

ConsumerController::ConsumerController(
	TSQueue<Item*>* worker_queue,
	TSQueue<Item*>* writer_queue,
	Transformer* transformer,
	int check_period,
	int low_threshold,
	int high_threshold
) : worker_queue(worker_queue),
	writer_queue(writer_queue),
	transformer(transformer),
	check_period(check_period),
	low_threshold(low_threshold),
	high_threshold(high_threshold) {
}

ConsumerController::~ConsumerController() {
	std::cout<<"Scaling down consumers to 0\n";
}

void ConsumerController::start() {
	// TODO: starts a ConsumerController thread
	pthread_create(&t, 0, ConsumerController::process, (void*)this);
}

void* ConsumerController::process(void* arg) {
	// TODO: implements the ConsumerController's work
	ConsumerController* consumerController = (ConsumerController*)arg;

	Consumer* first_consumer = new Consumer(consumerController->worker_queue, consumerController->writer_queue, consumerController->transformer);
	int old_size = consumerController->consumers.size();
	first_consumer->start();
	consumerController->consumers.push_back(first_consumer);
	int new_size = consumerController->consumers.size();
	std::cout<<"Scaling up consumers from " << old_size << " to " << new_size << std::endl;

	unsigned long long counter = 0;
	unsigned long long time = 0;
	while(true){
		usleep(consumerController->check_period);
		time+=consumerController->check_period;
		int queue_size = consumerController->worker_queue->get_size();
		std::cout<<"time: "<<time<<" queue_size: "<<queue_size<<std::endl;
		if(queue_size > (consumerController->high_threshold) ){
			Consumer* consumer = new Consumer(consumerController->worker_queue, consumerController->writer_queue, consumerController->transformer);
			old_size = consumerController->consumers.size();
			consumer->start();
			consumerController->consumers.push_back(consumer);
			new_size = consumerController->consumers.size();
			std::cout<<"Scaling up consumers from " << old_size << " to " << new_size << std::endl;
		}
		else if( (queue_size < consumerController->low_threshold) && (consumerController->consumers.size()>1) ){
			Consumer* consumer = consumerController->consumers.back();
			old_size = consumerController->consumers.size();
			consumer->cancel();
			consumerController->consumers.pop_back();
			new_size = consumerController->consumers.size();
			std::cout<<"Scaling down consumers from " << old_size << " to " << new_size << std::endl;
		}
	}
	
	old_size = consumerController->consumers.size();
	first_consumer->cancel();
	consumerController->consumers.pop_back();
	new_size = consumerController->consumers.size();
	std::cout<<"Scaling down consumers from " << old_size << " to " << new_size << std::endl;

	return nullptr;
}

#endif // CONSUMER_CONTROLLER_HPP
