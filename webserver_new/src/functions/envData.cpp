#include "../../include/classes/EnvpWrapper.hpp"

EnvpWrapper& envData(){
	static EnvpWrapper data;
	return (data);
}
