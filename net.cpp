#include <iostream> // console output
#include <vector>
#include <cmath> // used for rounding, etc.
#include <fstream> // file input/output
#include <cstdlib> // used to execute shell commands
#include <sstream> // build (append) strings
#include <string> // cast to string objects
#include <algorithm> // for remove-by-value remove()
#include <queue> // for breadth-first optimal path precomputation
#include <unistd.h> // operating system-specific c++ functions
#include <limits>

using namespace std;

class Node {
	public:
		int 				ID;
		vector<Node*> 	connections;
		vector<int> 	packets;
		bool				checked; // used when we evaluate if there is a path from Start to End so we don't traverse in loops
		Node*				efficientNeighbor; // the neighbor that goes to End

		/// Returns: true, if the requested node is one of the ones in this node's connections.
		bool isAConnection(Node* target) {
			for (int nodeIdx=connections.size()-1; nodeIdx>=0; --nodeIdx) {
				if (target == connections[nodeIdx]) {
					return true;
				}
			}
			return false;
		}

		/// Returns: true, if through some sequence of nodes, it can reach the specified end node, form this node.
		/// Does so by calling the same function again, for each connected node.
		/// Eventually, we should reach the end node.
		bool canGetToEnd(Node* End) {
			if (this->checked)
				return false;
			if (this == End)
				return true;
			else
				this->checked = true;
			for (int i=connections.size()-1; i>=0; --i) {
				if (connections[i]->canGetToEnd(End))
					return true;
			}
			this->checked = false;
			return false;
		}
};

/// given a verctor of nodes, exports their graph representation to
/// a graphviz dot file, given a target filename to save as.
void saveToDot(vector<Node*>& nodes, const char* filename, vector<Node*>& path, Node* unsafeNode=NULL) {
	ofstream dotfile;
	dotfile.open(filename, ios::out);
	if (!dotfile.is_open()) {
		cerr << "couldn't open dot file: " << filename << endl;
		return;
	}
	dotfile << "digraph G {" << endl;
	for (int nodeIdx=nodes.size()-1; nodeIdx>=0; --nodeIdx) {
		for (int connIdx=nodes[nodeIdx]->connections.size()-1; connIdx>=0; --connIdx) {
			dotfile << nodes[nodeIdx]->ID << " -> " << nodes[nodeIdx]->connections[connIdx]->ID << ";" << endl;
		}
	}
	dotfile << nodes[0]->ID << " [shape=circle, style=filled, fillcolor=green];" << endl;
	dotfile << nodes[nodes.size()-1]->ID << " [shape=circle, style=filled, fillcolor=green];" << endl;
	for (int pi=0; pi<path.size(); pi++) {
		dotfile << path[pi]->ID << " [shape=circle, style=filled, fillcolor=yellow];" << endl;
	}
	if (unsafeNode != NULL) { // also overwrites the path node colorings
		dotfile << unsafeNode->ID << " [shape=circle, style=filled, fillcolor=red];" << endl;
	}
	dotfile << "}" << endl;
	dotfile.close();
}

/// executes the command line graphviz program to convert
/// a .dot file into a rendered png of the graph.
void makePNG(const char* filename, const char* pngFilename) {
	if (system(NULL)) { // if we have shell access
		stringstream cmd;
		cmd << "dot -Tpng " << filename << " > " << pngFilename;
		system(cmd.str().c_str());
	}
}

/// executes the MAC OS X system command to display
/// a png file, given a filename.
void showPNG(const char* filename) {
	if (system(NULL)) {
		stringstream cmd;
		cmd << "open " << filename;
		system(cmd.str().c_str());
	}
}

bool fileExists(const char* filename) {
	ifstream ifile(filename);
	return(ifile);
}

///
/// MAIN ENTRYPOINT
///
int main(int argc, char* argv[]) {
	srand(getpid()); // seed the random number generator
	//cout << getpid() << endl; // useful if there is an intermittent problem
	//srand(10301); // useful if there is an intermittent problem
	vector<Node*> nodes;
	vector<Node*> possibleTargets;
	int networkSize = 4; // how many nodes to simulate - default value
	int packetsInTheNetwork=3; // how many packets to simulate - default value
	float randomnessProbability=0.0; // how often a packet being sent from a node, takes a random path.
	bool shouldExportGraph=false;
	int networkSparseness=6; // decrease to make more connected (small optimal paths), increase to make less connected (longer optimal paths)


	// get CLI parameters
	if (argc >= 2) { // get network size from CLI (command line interface)
		networkSize = atoi(argv[1]);
	}
	if (argc >= 3) { // get packet quantity from CLI
		packetsInTheNetwork = atoi(argv[2]);
	}
	if (argc >= 4) { // get randomness probability from CLI
		randomnessProbability = atof(argv[3]);
	}
	if (argc >= 5) { // get graph export option
		if (string(argv[4]) == string("--graph")) {
			shouldExportGraph=true;
		}
	}

	// create nodes
	// and assign unique IDs
	for (int nodeIndex=0; nodeIndex<networkSize; nodeIndex++) {
		Node* node = new Node();
		node->ID = nodeIndex;
		node->efficientNeighbor=NULL;
		nodes.push_back(node);
	}

	// get references of special nodes
	Node* Start 	= nodes[0]; // the first node
	Node* End 		= nodes[nodes.size()-1]; // the very last node
	Node* Bad;		// the surveillance node
	int badCounter = 0; // how many packets were maliciously copied

	// connect nodes randomly to make a random network
	bool badNetwork; // used to restart network wiring
	do {
		badNetwork = false;

		// reset the network (clear all connections, and clear checked status)
		for (int nodeIdx=nodes.size()-1; nodeIdx>=0; --nodeIdx) {
			nodes[nodeIdx]->connections.clear();
			nodes[nodeIdx]->checked = false;
		}

		int connectionsToMake=(networkSize*networkSize - networkSize)/2 / networkSparseness; // set slightly lower (-2) than fully connected
		for (int connectionCounter=connectionsToMake-1; connectionCounter>=0; --connectionCounter) {
			// pick nodes by index to create a connection
			int fromNodeIndex;
			int toNodeIndex;
			bool tryAgain;
			do {
				tryAgain = false; // reset tryAgain for this loop iteration
				fromNodeIndex = rand() % nodes.size();
				toNodeIndex = rand() % nodes.size();
				if (fromNodeIndex == toNodeIndex) {
					tryAgain = true;
				}
				if (nodes[fromNodeIndex]->isAConnection(nodes[toNodeIndex])) {
					tryAgain = true;
				}
			} while (tryAgain); // keep picking until we find two different nodes
			// create the connection
			nodes[fromNodeIndex]->connections.push_back(nodes[toNodeIndex]); // forward connection
			nodes[toNodeIndex]->connections.push_back(nodes[fromNodeIndex]); // backward connection
		}
		for (int nodeIdx=nodes.size()-1; nodeIdx>=0; --nodeIdx) {
			if (nodes[nodeIdx]->connections.size() == 0) {
				badNetwork = true;
				break;
			}
		}

		// make sure there is only one connection to and from the start/end nodes
		// remove all others
		// this ensures sort of more interesting networks
		vector<Node*>* oc; // other's connections
		// remove connections with Start
		if (Start->connections.size() > 1) {
			for (int cidx=Start->connections.size()-1; cidx>=1; --cidx) {
				oc = &Start->connections[cidx]->connections; // make a shortcut
				oc->erase(remove(oc->begin(), oc->end(), Start), oc->end()); // remove connection from neighbor to Start
				Start->connections.erase(Start->connections.begin()+cidx); // remove connection from Start to neighbor
			}
		}
		// remove connections with End
		if (End->connections.size() > 1) {
			for (int cidx=End->connections.size()-1; cidx>=1; --cidx) {
				oc = &End->connections[cidx]->connections; // make a shortcut
				oc->erase(remove(oc->begin(), oc->end(), End), oc->end()); // remove connection from neighbor to End
				End->connections.erase(End->connections.begin()+cidx); // remove connection from End to neighbor
			}
		}

		// if we can't get from Start to End, then make sure to try making a network again
		if (!Start->canGetToEnd(End)) {
			badNetwork = true;
		}
		for (int nodeIdx=nodes.size()-1; nodeIdx>=0; --nodeIdx) {
			nodes[nodeIdx]->checked = false;
		}

		if (Start->connections.size() == 0) {
			badNetwork = true;
			continue;
		}
		if (End->connections.size() == 0) {
			badNetwork = true;
			continue;
		}

		// Make sure nodes know the most efficient paths to End node
		queue<Node*> q; // breadth-first search queue
		q.push(End); // seed the queue, so the algorithm has something to start with
		//End->checked = true;
		while(q.size() > 0) {
			Node& n= *q.front(); // read first in line
			q.pop(); 				// remove first in line
			if (n.checked == true)
				continue;
			for (int nni=n.connections.size()-1; nni>=0; --nni) {
				if (n.connections[nni]->efficientNeighbor == NULL) {
					n.connections[nni]->efficientNeighbor = &n;
					q.push(n.connections[nni]);
				}
				n.checked = true;
			}
			
			//// old version - still worked?
			//for (int nni=n.connections.size()-1; nni>=0; --nni) {
			//	if (n.connections[nni]->checked == false) {
			//		n.connections[nni]->efficientNeighbor = &n;
			//		q.push(n.connections[nni]);
			//		n.connections[nni]->checked = true;
			//		if (&n == Start)
			//			cout << "looking at node 0" << endl;
			//	} 
			//}
		}

		//cout << "all optimal connections" << endl;
		//for (int i=0; i<nodes.size()-1; ++i) {
		//	if (nodes[i]->efficientNeighbor != NULL) {
		//		cout << nodes[i]->ID << " -> " << nodes[i]->efficientNeighbor->ID << endl;
		//	}
		//}

		possibleTargets.clear();
		Node* node = nodes[0]->connections[0];
		while(node != End) {
			possibleTargets.push_back(node);
			node = node->efficientNeighbor;
		}
		if (possibleTargets.size() <= 2) { // let's make sure we can't choose the second or penultimate nodes to be fair
			badNetwork = true;					// since those only have one connection to the end nodes and must transport packets
		} else {
			Bad = possibleTargets[rand()%(possibleTargets.size()-2)+1]; // pick a random surveillance node
		}
		//cout << "optimal path length: " << possibleTargets.size()+1 << endl;
		
	} while(badNetwork);


	/// Simulate the Network!
	int time=0;
	// create packets
	for (int packetIdx=packetsInTheNetwork-1; packetIdx>=0; --packetIdx) {
		Start->packets.push_back(time); // add packets to the network of this time
	}
	vector<int> packetsToRemove;
	do { // main time loop
		for (int nodeIdx=nodes.size()-1; nodeIdx>=0; --nodeIdx) { // process all nodes
			//cout << "time " << time << "processing node " << nodeIdx << endl;
			if ((nodes[nodeIdx] != End) && (nodes[nodeIdx]->packets.size() != 0)) { // if node has packets (and is not End node)
				//cout << "\tnot at the end and there are packets" << endl;
				int packetsProcessedThisNode=0;
				packetsToRemove.clear(); // get ready to remember packets we need to remove from this node (the processed ones)
				for (int packetIdx=0; packetIdx<nodes[nodeIdx]->packets.size(); packetIdx++) { // for all packets on the node...
					//cout << "\tprocessing packet " << packetIdx;
					if (abs(nodes[nodeIdx]->packets[packetIdx]) == time) { // only process packets that are marked to be processed this time frame
						//cout << " which should be processed (" << time << ")" << endl;
						if (packetsProcessedThisNode < 1) { // how many packets each node can process
							packetsProcessedThisNode++;
							if (nodes[nodeIdx]->connections.size() != 0) { // just making sure there's somewhere to send this
								if (nodes[nodeIdx]->packets[packetIdx]==0)
									nodes[nodeIdx]->packets[packetIdx]++;
								else {
									nodes[nodeIdx]->packets[packetIdx]=nodes[nodeIdx]->packets[packetIdx]+nodes[nodeIdx]->packets[packetIdx]/abs(nodes[nodeIdx]->packets[packetIdx]);
								}
								if (nodes[nodeIdx] == Bad) {
									if (nodes[nodeIdx]->packets[packetIdx] >= 0) {
										badCounter++;
										nodes[nodeIdx]->packets[packetIdx]*=-1;
									}
								}
								if ((float)rand()/(float)RAND_MAX < randomnessProbability) {
									// move packet randomly
									int toConnection=rand()%nodes[nodeIdx]->connections.size(); // pick a random connection to forward the packet
									nodes[nodeIdx]->connections[toConnection]->packets.push_back(nodes[nodeIdx]->packets[packetIdx]);
								} else {
									// move packet intelligently
									nodes[nodeIdx]->efficientNeighbor->packets.push_back(nodes[nodeIdx]->packets[packetIdx]); // move to next best node
								}
								packetsToRemove.push_back(packetIdx);
							} else { // we should never get here, but just in case... could help debugging later!
								cerr << " Error, reached a sink node (no outlet)" << endl;
								return (1);
							}
						} else {
							if (nodes[nodeIdx]->packets[packetIdx]==0)
								nodes[nodeIdx]->packets[packetIdx]++;
							else {
								nodes[nodeIdx]->packets[packetIdx]=nodes[nodeIdx]->packets[packetIdx]+nodes[nodeIdx]->packets[packetIdx]/abs(nodes[nodeIdx]->packets[packetIdx]);
							}
						}
					}
					else {
						//cout << " which is being skipped (" << time << ")" << endl;
					}
				}
				for (int x=packetsToRemove.size()-1; x>=0; --x) {
					nodes[nodeIdx]->packets.erase(nodes[nodeIdx]->packets.begin()+packetsToRemove[x]);
				}
			}
		}

		/// shows the network step by step state of each node
		//cout << "state of the network after time " << time << endl;
		//for (int nidx=0; nidx<nodes.size(); ++nidx) {
		//	cout << nidx << "[";
		//	for (int pidx=0; pidx<nodes[nidx]->packets.size(); ++pidx) {
		//		cout << nodes[nidx]->packets[pidx] << ", ";
		//	}
		//	cout << "] ";
		//}
		//cout << endl;
		//cin.ignore(numeric_limits<streamsize>::max(), '\n');

		time++;
	} while(End->packets.size() != packetsInTheNetwork);
	ofstream data;
	if (fileExists("data.csv") == false) {
		data.open("data.csv", ofstream::out | ofstream::app);
		data << "network size, packets sent, randomness probability, time for all packets to reach destination, optimal path length, packets copied by surveillance node, security (%)" << endl;
	} else {
		data.open("data.csv", ofstream::out | ofstream::app);
	}
	data << networkSize << ", " << packetsInTheNetwork << ", " << randomnessProbability << ", " << time << ", " << possibleTargets.size()+1 << ", " << badCounter << ", " << (((float)(packetsInTheNetwork-badCounter)*100.0f)/(float)packetsInTheNetwork) << endl;
	data.close();
	cout << endl;
	cout << "took " << time << " ticks to send " << packetsInTheNetwork << " packets." << endl;
	cout << badCounter << " packets captured by insecure node meaning" << endl;
	cout << (((float)(packetsInTheNetwork-badCounter)*100.0f)/(float)packetsInTheNetwork) << "% security in a network with an optimal path of length " << possibleTargets.size()+1 << endl;
	if (shouldExportGraph) {
		saveToDot(nodes, "graph.dot", possibleTargets, Bad);
	}
	//makePNG("graph.dot","graph.png");
	//showPNG("graph.png");
	return(0);
}
