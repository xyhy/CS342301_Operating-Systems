#include <assert.h>
#include <stdlib.h>
#include "ts_queue.hpp"
#include "item.hpp"
#include "reader.hpp"
#include "writer.hpp"
#include "producer.hpp"
#include "consumer_controller.hpp"

#define READER_QUEUE_SIZE 200
#define WORKER_QUEUE_SIZE 200
#define WRITER_QUEUE_SIZE 4000
#define CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE 20
#define CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE 80
#define CONSUMER_CONTROLLER_CHECK_PERIOD 10000

int main(int argc, char** argv) {
	assert(argc == 4);

	int n = atoi(argv[1]);
	std::string input_file_name(argv[2]);
	std::string output_file_name(argv[3]);

	// TODO: implements main function
	TSQueue<Item*>* reader_queue;
	TSQueue<Item*>* worker_queue;
	TSQueue<Item*>* writer_queue;

	reader_queue = new TSQueue<Item*>(READER_QUEUE_SIZE);
	worker_queue = new TSQueue<Item*>(WORKER_QUEUE_SIZE);
	writer_queue = new TSQueue<Item*>(WRITER_QUEUE_SIZE);

	Transformer* transformer = new Transformer;

	Reader* reader = new Reader(n, input_file_name, reader_queue);
	Writer* writer = new Writer(n, output_file_name, writer_queue);

	Producer* p1 = new Producer(reader_queue, worker_queue, transformer);
	Producer* p2 = new Producer(reader_queue, worker_queue, transformer);
	Producer* p3 = new Producer(reader_queue, worker_queue, transformer);
	Producer* p4 = new Producer(reader_queue, worker_queue, transformer);

	ConsumerController* consumerController = new ConsumerController(
		worker_queue,
		writer_queue,
		transformer,
		CONSUMER_CONTROLLER_CHECK_PERIOD,
		WORKER_QUEUE_SIZE/100*CONSUMER_CONTROLLER_LOW_THRESHOLD_PERCENTAGE,
		WORKER_QUEUE_SIZE/100*CONSUMER_CONTROLLER_HIGH_THRESHOLD_PERCENTAGE
	);

	reader->start();
	writer->start();

	p1->start();
	p2->start();
	p3->start();
	p4->start();


	consumerController->start();

	
	reader->join();
	writer->join();
	// consumerController->cancel();
	// consumerController->join();
	


	
	delete p1;
	delete p2;
	delete p3;
	delete p4;
	delete writer;
	delete reader;
	delete transformer;
	delete reader_queue;
	delete worker_queue;
	delete writer_queue;
	delete consumerController;
	
	// pthread_exit(0);

	return 0;
}
