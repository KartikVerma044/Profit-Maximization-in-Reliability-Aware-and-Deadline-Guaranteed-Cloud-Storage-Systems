#include <bits/stdc++.h>
using namespace std;

int grid_size = 100;
int max_service_rate = 501;
int min_service_rate = 500;
int max_server_storage_capacity = 51;
int min_server_storage_capacity = 50;
int max_data_partition_storage_size = 11;
int min_data_partition_storage_size = 10;
double max_server_reliability = 1.00;
double min_server_reliability = 0.01;

int max_storage_cost_per_unit = 1000;
int min_storage_cost_per_unit = 500;
int max_maintainence_cost_per_request = 201;
int min_maintainence_cost_per_request = 200;

double alpha = 4;
double probability_of_max_alpha_data_partitions = 0.99;
double max_tenant_reliability = 1.00;
double min_tenant_reliability = 0.01;
double max_tenant_deadline = 1.00;
double min_tenant_deadline = 0.10;

int num_initial_servers = 1000;
int num_data_partitions = 2500;
int num_tenants = 2;

int max_num_requests = 50000;
int num_intervals = 150;
int num_requests = 20000;

double profit;
double init_cost;
double dt_cost;
double storage_cost;
double service_cost;

double successful_requests;

struct DataPartition{
	int storage_capacity_required;
	double charge_per_request;
	DataPartition(){
		storage_capacity_required = min_data_partition_storage_size + rand()%(max_data_partition_storage_size-min_data_partition_storage_size);
		charge_per_request = storage_capacity_required;
	}
};

struct Tenant{
	double required_reliability;
	double required_deadline;
	int deadline_strictness;
	double tenant_f1;
	Tenant(int id){
		if(id==0)
		{
		required_deadline = 0.6;
		required_reliability = 0.8;
		}
		// else if(id==1)
		// {
		// required_deadline = 0.75;
		// required_reliability = 0.6;
		// }
		else
		{
		required_deadline = 0.9;
		required_reliability = 0.45;
		}
		deadline_strictness = -log(1.0-pow((1-required_reliability)/probability_of_max_alpha_data_partitions,1.0/alpha))/required_deadline;
		tenant_f1 = pow((1-required_reliability)/probability_of_max_alpha_data_partitions,1.0/alpha);
	}
};

struct Request{
	int tenant;
	int ts;
    vector<int> requested_data_partitions;
	Request(int dataid, int timestamp, int userid=rand()){
		tenant=userid%num_tenants;
		// for(int j=0;j<num_data_partitions;j++)
		// {
		// 	double prob=1.0*rand()/RAND_MAX;
		// 	if(prob>0.6)
		// 	{
		// 		requested_data_partitions.push_back(j);
		// 	}
		// }
		requested_data_partitions.push_back(dataid);
		ts=timestamp;
	}
};

struct Server{
	int x_coordinate;
	int y_coordinate;
    int storage_capacity;
    int initial_occupied_storage;
    int occupied_storage;
	int service_rate;
    int deadline_guaranteed_request_arrival_rate;
    int request_arrival_rate;
	double initial_server_reliability;
	double server_reliability;
	int unsuccessful_storage;
	int unsuccessful_network;
	double initialization_cost;
	double storage_cost_per_unit;
	double maintainence_cost_per_request;
	vector<double> execution_cost;
	unordered_set<int> tenants;
	vector<pair<int,int>> request_queue;
    unordered_set<int> initial_available_data_partitions;
    unordered_set<int> available_data_partitions;
    // vector<int> data_partitions_request_arrival_rate;
	vector<int> backups;
    int available_service_capacity(){
        return deadline_guaranteed_request_arrival_rate-request_arrival_rate;
    }
	int available_storage_capacity(){
        return storage_capacity-occupied_storage;
    }
	Server(){
		x_coordinate=rand()%grid_size;
		y_coordinate=rand()%grid_size;
		service_rate=min_service_rate+rand()%(max_service_rate-min_service_rate); // KOI TENANT NAHI HAI TOH DEADLINE GURANTEED IS VERY HIGH
		deadline_guaranteed_request_arrival_rate=service_rate;
		storage_capacity=min_server_storage_capacity+rand()%(max_server_storage_capacity-min_server_storage_capacity);
		initial_server_reliability=server_reliability=min_server_reliability+(max_server_reliability-min_server_reliability)*rand()/RAND_MAX;
		initialization_cost=service_rate*50+storage_capacity*1000;
		storage_cost_per_unit=min_storage_cost_per_unit+rand()%(max_storage_cost_per_unit-min_storage_cost_per_unit);
		maintainence_cost_per_request=min_maintainence_cost_per_request+0*(max_maintainence_cost_per_request-min_maintainence_cost_per_request);
	}
};


vector<Server*> servers_backup; // stores all the servers
vector<Server*> servers; // stores all the servers
vector<Request*> incoming_requests_backup;
vector<Request*> incoming_requests;
vector<DataPartition*> data_partitions;
vector<Tenant*> tenants;

int servers_used=0;
int requests_used=0;

bool sortRandomly(const Server* a,const Server* b){
	return rand()%2;
}
bool sortByReliability(const Server* a,const Server* b){
	return a->server_reliability > b->server_reliability;
}
bool sortByAvailableServiceCapacity(Server* a,Server* b){
	return a->available_service_capacity() > b->available_service_capacity();
}
bool sortByCost(const Server* a, const Server* b){
	return a->server_reliability*a->maintainence_cost_per_request<b->server_reliability*b->maintainence_cost_per_request;
}


/*int initialHardwareFaultPrediction() {
	for(int i=0;i<servers.size();i++)
	{
		int fault1=rand()/RAND_MAX; // storage fault
		int fault2=rand()/RAND_MAX; // network fault
		servers[i]->server_reliability=e^-(fault1+fault2);
	}
	return 0;
}
int hardwareFaultPrediction() {
	for(int i=0;i<servers.size();i++)
	{
		int fault1=servers[i]->unsuccessfulStorage/total;
		int fault2=servers[i]->unsuccessfulNetwork/total;
		servers[i]->server_reliability=e^-(fault1+fault2);
	}
	return 0;
}*/

void newReplicaAllocation(Request* incoming_request, int data_partition_id){
	// cout<<"newReplicaAllocation"<<endl;
	bool request_alloted=0;
	for(int i=0; i<servers.size(); i++){
		if(servers[i]->available_service_capacity()>0 && servers[i]->server_reliability >= tenants[incoming_request->tenant]->required_reliability && servers[i]->available_storage_capacity()>=data_partitions[data_partition_id]->storage_capacity_required){
            if(servers[i]->tenants.find(incoming_request->tenant)!=servers[i]->tenants.end()){
                //create a data replica here and add request
                servers[i]->request_queue.push_back({incoming_request->tenant,data_partition_id});
                servers[i]->available_data_partitions.insert(data_partition_id);
                servers[i]->occupied_storage+=data_partitions[data_partition_id]->storage_capacity_required;
                servers[i]->request_arrival_rate++;
				request_alloted=1;
				// profit-=servers[i]->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
				// dt_cost+=servers[i]->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
				//cout<<"New Data Partition "<<data_partition_id<<" created and alloted at server "<<i<<endl;
				break;
            }
            else{
                int new_deadline_guaranteed_request_arrival_rate=min(servers[i]->deadline_guaranteed_request_arrival_rate, servers[i]->service_rate-tenants[incoming_request->tenant]->deadline_strictness);
                if(servers[i]->request_arrival_rate < new_deadline_guaranteed_request_arrival_rate){
                    //create a data replica here and add request and tenant
					servers[i]->deadline_guaranteed_request_arrival_rate = new_deadline_guaranteed_request_arrival_rate;
                    servers[i]->request_queue.push_back({incoming_request->tenant,data_partition_id});
                    servers[i]->available_data_partitions.insert(data_partition_id);
                    servers[i]->occupied_storage+=data_partitions[data_partition_id]->storage_capacity_required;
                    servers[i]->request_arrival_rate++;
                    servers[i]->tenants.insert(incoming_request->tenant);
					request_alloted=1;
					// profit-=servers[i]->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
					// dt_cost+=servers[i]->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
					//cout<<"New Data Partition "<<data_partition_id<<" created and alloted at server "<<i<<endl;
					break;
                }
            }
        }
	}
	if(!request_alloted){
		//create a new server and add data replica here and add request and tenant 
		// cout<<"New Server"<<endl;
		Server* new_server = servers_backup[servers_used++];
		new_server->server_reliability=max(new_server->server_reliability, tenants[incoming_request->tenant]->required_reliability+(1-tenants[incoming_request->tenant]->required_reliability)*rand()/RAND_MAX);
		new_server->request_queue.push_back({incoming_request->tenant,data_partition_id});
		if(new_server->available_data_partitions.find(data_partition_id)==new_server->available_data_partitions.end()){
			new_server->available_data_partitions.insert(data_partition_id);
			new_server->occupied_storage+=data_partitions[data_partition_id]->storage_capacity_required;
		}
		new_server->request_arrival_rate++;
		new_server->deadline_guaranteed_request_arrival_rate = new_server->service_rate-tenants[incoming_request->tenant]->deadline_strictness;
		new_server->tenants.insert(incoming_request->tenant);
		servers.push_back(new_server);
		profit-=new_server->initialization_cost;
		init_cost+=new_server->initialization_cost;
		// profit-=new_server->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
		// dt_cost+=new_server->storage_cost_per_unit*data_partitions[data_partition_id]->storage_capacity_required;
	}
}

void helper(Request* incoming_request, int data_partition_id)
{
	int request_alloted=0;
	for(int i=0;i<servers.size();i++)
	{
		if(find(servers[i]->available_data_partitions.begin(),servers[i]->available_data_partitions.end(),data_partition_id)!=servers[i]->available_data_partitions.end()) //DATA PARTITION EXISTS.
		{
			if(find(servers[i]->tenants.begin(),servers[i]->tenants.end(),incoming_request->tenant)!=servers[i]->tenants.end()) // TENANT ALREADY SERVED
			{
				if(servers[i]->available_service_capacity()>0 && servers[i]->server_reliability >= tenants[incoming_request->tenant]->required_reliability)
				{
					servers[i]->request_arrival_rate++;
					servers[i]->request_queue.push_back({incoming_request->tenant,data_partition_id});
					request_alloted=1;
					//cout<<"Data Partition "<<data_partition_id<<" alloted at server "<<i<<endl;
					break;
				}
			}
			else // tenant not yet served so we have to recalculate deadline_guranteed
			{
				// Calculate the new deadline guranteed rate if this tenant is served.
				int new_deadline_guaranteed_request_arrival_rate=min(servers[i]->deadline_guaranteed_request_arrival_rate, servers[i]->service_rate-tenants[incoming_request->tenant]->deadline_strictness);
				if((servers[i]->request_arrival_rate < new_deadline_guaranteed_request_arrival_rate) && servers[i]->server_reliability >= tenants[incoming_request->tenant]->required_reliability)
				{	
					servers[i]->deadline_guaranteed_request_arrival_rate = new_deadline_guaranteed_request_arrival_rate;
					servers[i]->request_arrival_rate++;
					servers[i]->tenants.insert(incoming_request->tenant);
					servers[i]->request_queue.push_back({incoming_request->tenant,data_partition_id});
					request_alloted=1;
					//cout<<"Data Partition "<<data_partition_id<<" alloted at server "<<i<<endl;
					break;
				}
			}
		}
	}
	if(!request_alloted) newReplicaAllocation(incoming_request, data_partition_id);
}

void requestAllotment(Request* incoming_request,int algorithm)
{
	// cout<<"requestAllotment"<<endl;
	if(algorithm == 1) // FCFS
	{
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			helper(incoming_request, k);
		}
	}
	else if(algorithm==2) //Random
	{
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			// sort(servers.begin(),servers.end(),sortRandomly);
			random_shuffle(servers.begin(),servers.end());
			helper(incoming_request, k);
		}
	}
	else if(algorithm==3){ //max reliablity first
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByReliability);
			helper(incoming_request, k);
		}
	}
	else if(algorithm==4){ //min reliablity first
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByReliability);
			reverse(servers.begin(),servers.end());
			helper(incoming_request, k);
		}
	}
	else if(algorithm==5){ //max available service capacity first
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByAvailableServiceCapacity);
			helper(incoming_request, k);
		}
	}
	else if(algorithm==6){ //min available service capacity first
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByAvailableServiceCapacity);
			reverse(servers.begin(),servers.end());
			helper(incoming_request, k);
		}
	}
	else if(algorithm==7){ //greedy
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByCost);
			helper(incoming_request, k);
		}
	}
	else if(algorithm==8){ //reverse greedy
		for(int k=0;k<incoming_request->requested_data_partitions.size();k++)
		{
			sort(servers.begin(),servers.end(),sortByCost);
			reverse(servers.begin(),servers.end());
			helper(incoming_request, k);
		}
	}
}

void workloadConsolidation()
{
	vector <pair<double,int>> utilization;
	for(int i=0;i<servers.size();i++)
	{
		utilization.push_back(make_pair(servers[i]->request_arrival_rate/servers[i]->deadline_guaranteed_request_arrival_rate,i));
	}
	sort(utilization.begin(),utilization.end());
	for(int i=0;i<utilization.size();i++)
	{
		Server* underloaded = servers[utilization[i].second];
		//MOVE UNDERLOADED SERVER TO ANOTHER SERVER
	}
}

void serverPairing() // EVERY SERVER HAS 3 BACKUPS AND EACH SERVER ONLY CHECKS ON PROXIMITY PARAMETER
{	
	vector<pair<int,int>> pair;
	map<int,int> count_pairs;
	for(int i=0;i<servers.size();i++)
	{	
		for(int j=0;j<servers.size();j++)
		{
			if(i != j)
			{
				pair.push_back(make_pair((servers[i]->x_coordinate-servers[j]->x_coordinate)*(servers[i]->x_coordinate-servers[j]->x_coordinate)+(servers[i]->y_coordinate-servers[j]->y_coordinate)*(servers[i]->y_coordinate-servers[j]->y_coordinate),j));
			}
		}
		sort(pair.begin(),pair.end());
		int count=0;
		for(int k=0;k<pair.size();k++)
		{
			if(count_pairs[pair[k].second]<3)
			{
				servers[i]->backups.push_back(pair[k].second);
				count_pairs[pair[k].second]++;
				count++;
				if(count==3)
				break;
			}
		}
		pair.clear();
	}
}

int serverWakeUp()
{
	return 1;
}	

void initializeDataPartitions(){
	// cout<<"initializeDataPartitions"<<endl;
	data_partitions.clear();
	for(int i=0; i<num_data_partitions; i++){
		DataPartition* new_data_partition = new DataPartition();
		data_partitions.push_back(new_data_partition);
	}
}

void initializeTenants(){
	// cout<<"initializeTenants"<<endl;
	tenants.clear();
	for(int i=0; i<num_tenants; i++){
		Tenant* new_tenant = new Tenant(i);
		// cout<<new_tenant->required_deadline<<" "<<new_tenant->required_reliability<<" "<<new_tenant->deadline_strictness<<endl;
		tenants.push_back(new_tenant);
	}
}

void calculateExecutionCost()
{
	// cout<<"calculateExecutionCost"<<endl;
	for(int i=0;i<servers.size();i++) // 10 servers
	{
		for(int j=0;j<num_data_partitions;j++) // 10 data partitions
		{
			double cost=1.0*rand()/RAND_MAX;
			cost=cost*100;
			servers[i]->execution_cost.push_back(cost);
		}
	}
}

void storeDataPartitions()
{
	// cout<<"storeDataPartitions"<<endl;
	for(int i=0;i<servers_backup.size();i++)
	{
		for(int j=0;j<num_data_partitions;j++)
		{
			double prob=1.0*rand()/RAND_MAX;
			if(prob>0.5) // IF PROBABILITY IS GREATER THAN .5 THEN THE DATA PARTITION IS STORED IN THE SERVER
			{
				servers_backup[i]->initial_available_data_partitions.insert(j);
				servers_backup[i]->available_data_partitions.insert(j);
				servers_backup[i]->initial_occupied_storage+=data_partitions[j]->storage_capacity_required;
				servers_backup[i]->occupied_storage+=data_partitions[j]->storage_capacity_required;
			}
		}
	}
}

void initializeServers()
{
	// cout<<"initializeServers"<<endl;
	servers.clear();
	for(int i=0;i<num_initial_servers;i++)
	{
		Server* new_server = new Server();
		servers_backup.push_back(new_server);
	}
	// Data partitions are stored
	storeDataPartitions();
	// Calculate the execution cost for each server
	// calculateExecutionCost();
	// Calculate reliability for each server
	// initialHardwareFaultPrediction();
	// Create backup server list
	// serverPairing();
}

void requestGeneration()
{
	ifstream file;
    file.open("C:\\Users\\Vin-It\\Downloads\\BTP\\Dataset\\Spotify\\trace.csv");
    string line;
    getline(file, line);
    while (getline(file, line)) {
        int userid,dataid,timestamp,flag=0;
        string tmp="";
        for(int i=0;i<line.length();i++){
            if(line[i]==' '){
                if(flag==0) userid=stoi(tmp);
                if(flag==1) dataid=stoi(tmp);
                flag++;
                tmp="";
            }
            else tmp+=line[i];
        }
        timestamp=stoi(tmp);
		Request* new_request = new Request(dataid, timestamp, userid);
		if(new_request->requested_data_partitions.size()>0) incoming_requests_backup.push_back(new_request);
		// if(incoming_requests_backup.size()>50000) break;
    }
    file.close();

	// // cout<<"requestGeneration"<<endl;
	// int request_count = max_num_requests;
	// //cout<<"Req count = "<<request_count<<endl;
	// for(int i=0;i<request_count;i++)
	// {
	// 	Request* new_request = new Request();
	// 	if(new_request->requested_data_partitions.size()>0) incoming_requests_backup.push_back(new_request);
	// }
}

void executeRequest()
{
	// cout<<"executeRequest"<<endl;
	for(int i=0;i<servers.size();i++)
	{
		double server_successful_requests = 0;
		for(int j=0; j<servers[i]->request_queue.size();j++){
			if(servers[i]->server_reliability>1.0*rand()/RAND_MAX){
				double reliability = tenants[servers[i]->request_queue[j].first]->required_reliability;
				double deadline = tenants[servers[i]->request_queue[j].first]->required_deadline;
				double data_partition_cost = data_partitions[servers[i]->request_queue[j].second]->charge_per_request;
				// profit+=data_partition_cost*(1+reliability)/deadline;
				// profit+=data_partition_cost*exp(reliability)/deadline;
				profit+=5*data_partition_cost*(1+3*exp(reliability))/deadline;
				// profit+=data_partition_cost/((1-reliability)*deadline);
				server_successful_requests+=1;
				successful_requests+=1;
			}
		}
		// profit-=servers[i]->initialization_cost;
		profit-=servers[i]->server_reliability*servers[i]->request_queue.size()*servers[i]->maintainence_cost_per_request;
		service_cost+=servers[i]->request_queue.size()*servers[i]->maintainence_cost_per_request;
		if(servers[i]->request_queue.size())
		{	
			servers[i]->server_reliability=server_successful_requests/(1.0*servers[i]->request_queue.size());
		}
		// profit-=servers[i]->occupied_storage*servers[i]->storage_cost_per_unit;
		// storage_cost+=servers[i]->occupied_storage*servers[i]->storage_cost_per_unit;
		servers[i]->tenants.clear();
		servers[i]->request_queue.clear();
    	// servers[i]->available_data_partitions.clear();
		// servers[i]->occupied_storage=0;
		servers[i]->deadline_guaranteed_request_arrival_rate=servers[i]->service_rate;
		servers[i]->request_arrival_rate=0;
	}
} 

int main()
{
	// freopen("input.txt", "r", stdin); 
	// time_t t = time(0);
	// string s = ctime(&t);
	// s+=".txt";
    // freopen("btp_output.txt", "w", stdout);
	// Initialize 
	// cout<<"In main"<<endl;
	srand(time(0));
	initializeDataPartitions();
	initializeTenants();
	initializeServers();
	requestGeneration();
	// cout<<incoming_requests_backup.size()<<endl;
	// for(Server* s:servers_backup) servers.push_back(s);
	// for(int num_requests=200;num_requests<=2000;num_requests+=200){
	for(int algo=1;algo<9;algo++){
		if(algo==4 && algo==6 && algo==8) continue;
		// for(Server* s: servers_backup) servers.push_back(s);
		double earning=0;
		double success=0;
		double total_requests=0;
		init_cost=0;
		// dt_cost=0;
		storage_cost=0;
		service_cost=0;

		servers_used=0;
		requests_used=0;

		for(int loop=0;loop<num_intervals;loop++)
		{
			// Generate Requests from Tenants
			// requestGeneration();
			for(int i=0;i<num_requests;i++) 
			{
				if(requests_used<incoming_requests_backup.size() && incoming_requests_backup[requests_used]->ts==loop) incoming_requests.push_back(incoming_requests_backup[requests_used++]);
				else break;
			}
			// Request Allotment
			profit=0;
			successful_requests=0;
			total_requests=0;
			if(incoming_requests.size()==0) continue;
			for(int i=0;i<incoming_requests.size();i++)
			{
				// cout<<"Req No. = "<<i+1<<endl;
				//cout<<"Tenant = "<<incoming_requests[i]->tenant<<endl;
				//cout<<"Data partitions ";
				// for(int j:incoming_requests[i]->requested_data_partitions) //cout<<j<<" ";
				//cout<<endl;
				requestAllotment(incoming_requests[i],algo);
				total_requests+=incoming_requests[i]->requested_data_partitions.size();
				//cout<<"done"<<endl;
				// delete incoming_requests[i];
			}
			// cout<<loop;
			executeRequest();
			// cout<<incoming_requests.size();
			profit=profit/incoming_requests.size();
			successful_requests=successful_requests/total_requests;
			incoming_requests.clear();
			// cout<<"Profit = "<<profit<<" SuccessRatio = "<<successful_requests<<endl;
			earning+=profit;
			success+=successful_requests;
		}
		// cout<<init_cost/(num_requests*num_intervals)<<" - init "<<service_cost/(num_requests*num_intervals)<<" - svc "<<storage_cost/(num_requests*num_intervals)<<" - stc "<<profit<<endl;
		cout<<setfill(' ')<<setw(4)<<servers.size()<<", ";
		cout<<setfill(' ')<<setw(8)<<fixed<<setprecision(2)<<earning/num_intervals<<", ";
		cout<<fixed<<setprecision(6)<<success/num_intervals<<", ";
		// cout<<endl;
		// for(int i=0;i<servers.size();i++){
		// 	delete servers[i];
		// }
		for(int i=0;i<servers.size();i++){
			servers[i]->server_reliability=servers[i]->initial_server_reliability;
			servers[i]->available_data_partitions=servers[i]->initial_available_data_partitions;
			servers[i]->occupied_storage=servers[i]->initial_occupied_storage;
		}
		servers.clear();
	}
	cout<<endl;
}
	

/* result = 
[    
	 47, 337.86,0.854899,    47, 389.08,0.941156,    98, 435.61,0.969758,    48, 390.88,0.930129,    58, 443.07,0.930358,
	102, 347.82,0.868398,    97, 413.49,0.945218,   207, 429.08,0.968022,    91, 382.81,0.912128,    95, 434.47,0.932809, 
	147, 343.31,0.850482,   161, 405.89,0.941091,   283, 440.42,0.969075,   121, 377.76,0.889922,   158, 425.02,0.920986, 
	171, 338.78,0.828543,   210, 398.04,0.934969,   366, 432.63,0.969058,   157, 369.58,0.894213,   188, 432.52,0.910859, 
	233, 335.03,0.837115,   293, 409.06,0.940707,   475, 434.22,0.968869,   212, 377.91,0.898304,   246, 425.95,0.902009, 
	280, 335.49,0.850396,   331, 416.02,0.945624,   555, 436.68,0.969405,   250, 360.57,0.886080,   289, 439.40,0.931395, 
	330, 346.22,0.864371,   391, 412.75,0.940321,   608, 437.52,0.969219,   323, 378.68,0.911912,   343, 437.46,0.924706, 
	356, 351.70,0.868708,   445, 418.90,0.942266,   733, 434.37,0.969504,   388, 375.13,0.906165,   400, 421.19,0.907488, 
	434, 365.30,0.872895,   515, 413.85,0.942213,   826, 436.22,0.969222,   401, 398.34,0.908093,   420, 438.97,0.926086, 
	466, 350.36,0.862797,   571, 416.37,0.942721,   942, 434.48,0.969097,   464, 391.01,0.916143,   461, 430.73,0.915462,
]
  */