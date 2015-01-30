#include "eyeKin.h"
#include <Windows.h>
size_t personalRobotics::Tcp::socketCount = 0;

void main(int argC, char **argV)
{
	int a;
	personalRobotics::EyeKin eyeKin;
	while (true)
	{
		if (eyeKin.getServer()->connected())
		{
			// Generate serializable list
			eyeKin.generateSerializableList();

			// Lock list and generate serial data
			eyeKin.lockSerializableList();
			if (eyeKin.newSerializableListGenerated())
			{
				eyeKin.unsetNewSerializableListGeneratedFlag();

				// Send data over socket
				std::string outString;
				std::mutex bufferMutex;
				bufferMutex.lock();
				eyeKin.getSerializableList()->SerializeToString(&outString);
				int dataLenght = outString.size();
				int networkOrderDataLength = htonl(dataLenght);
				eyeKin.getServer()->write(4, (char*)&networkOrderDataLength);
				eyeKin.getServer()->asyncSend(dataLenght, &outString[0], &bufferMutex,false,true);
			}
		}
	}
	std::cin >> a;
}
