/* client.cc
 *
 * demonstration of gRPC client 
 *
 * Controlled Unclassified Information 
 *
 * Lockheed Proprietary
 *
 */

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
//#include <grpcpp/reflection.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/strings/str_format.h"
#include "absl/synchronization/mutex.h"

#include "messaging_service.grpc.pb.h"
#include "messaging_service.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::CallbackServerContext;
using grpc::EnableDefaultHealthCheckService;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerUnaryReactor;
using grpc::ServerContext;
using grpc::Status;
using grpc::StatusCode;
using messaging::MessageSender;
using messaging::CplTicketRequest;
using messaging::SendMessageResponse;

ABSL_FLAG( std::string, target, "localhost:50051", "Server address" );

struct Message {
  std::string aircraft_sn;
  std::string ofp_id;
  std::string jcn;
  std::string wce;
  std::string base_id;
  std::string unit_id;
  std::string creation_dtg;
};

struct Message msg;

class MessageClient 
{
public:
    MessageClient(std::shared_ptr<Channel> channel)
        : stub_(MessageSender::NewStub(channel)) {}

    SendMessageResponse SendMessage( const std::string& aircraft_sn, 
	    	      const std::string& ofp_id,
	    	      const std::string& jcn,
	    	      const std::string& wce,
	    	      const std::string& base_id,
	    	      const std::string& unit_id,
	    	      const std::string& creation_dtg )
//    SendMessageResponse SendMessage( struct Message msg )
    {
        // Create the request
        CplTicketRequest request;
	request.set_aircraft_sn( aircraft_sn );
	request.set_ofp_id( ofp_id );
	request.set_jcn( jcn );
	request.set_wce( wce );
	request.set_base_id( base_id );
	request.set_unit_id( unit_id );
	request.set_creation_dtg( creation_dtg );

        // Context for the RPC
        ClientContext context;
	// Add timeout to the context
	context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(5));

        // Response object
        SendMessageResponse response;

        // Call the server
        Status status = stub_->SendCplTicketRequest( &context, request, &response );

        // Process the response
        if (status.ok()) 
	{
		std::cout << "Client received reply message id: " << response.message_id() << std::endl;
		messaging::ErrorInfo error_info = response.error();
		std::cout << "Client received reply message: " << error_info.error_message() << std::endl;
		std::cout << "Client received reply code: " << error_info.response_code() << std::endl;
        } 
	else 
	{
            std::cout << "RPC failed: " << status.error_code() << ": " << status.error_message() << std::endl;
        }

	return response;
    }

private:
    std::unique_ptr<MessageSender::Stub> stub_;
};

constexpr char kRootCertificate[] =   "/home/mkelly/grpc/demo/ca.crt";
constexpr char kClientPrivateKey[] =  "/home/mkelly/grpc/demo/client.key";
constexpr char kClientCertificate[] = "/home/mkelly/grpc/demo/client.crt";

std::string LoadStringFromFile(std::string path) {
  std::ifstream file(path);
  if (!file.is_open()) {
    std::cout << "Failed to open " << path << std::endl;
    abort();
  }
  std::stringstream sstr;
  sstr << file.rdbuf();
  return sstr.str();
}

int main(int argc, char** argv) 
{
  absl::ParseCommandLine(argc, argv);
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  std::string target_str = absl::GetFlag(FLAGS_target);
  std::cout << target_str << std::endl;
  // Build a SSL options for the channel

  grpc::SslCredentialsOptions ssl_opts;
  ssl_opts.pem_root_certs = LoadStringFromFile(kRootCertificate);
  ssl_opts.pem_private_key = LoadStringFromFile(kClientPrivateKey);
  ssl_opts.pem_cert_chain = LoadStringFromFile(kClientCertificate);
  MessageClient sender( grpc::CreateChannel(target_str, grpc::SslCredentials( ssl_opts ) ) );

  std::string input;
  std::cout << "Enter aircraft sn: ";
  getline( std::cin, input );
  msg.aircraft_sn = input;
  std::cout << "Enter ofp id: ";
  getline( std::cin, input );
  msg.ofp_id = input;
  std::cout << "Enter jcn: ";
  getline( std::cin, input );
  msg.jcn = input;
  std::cout << "Enter wce: ";
  getline( std::cin, input );
  msg.wce = input;
  std::cout << "Enter base id: ";
  getline( std::cin, input );
  msg.base_id = input;
  std::cout << "Enter unit id: ";
  getline( std::cin, input );
  msg.unit_id = input;
  std::cout << "Enter creation dtg: ";
  getline( std::cin, input );
  msg.creation_dtg = input;

  SendMessageResponse reply = sender.SendMessage(msg.aircraft_sn, msg.ofp_id, msg.jcn, msg.wce, msg.base_id, msg.unit_id, msg.creation_dtg );

  return 0;
}
