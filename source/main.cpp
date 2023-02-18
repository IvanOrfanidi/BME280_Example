#include <chrono>
#include <iostream>
#include <thread>

#include <boost/program_options.hpp>

#include <bme280.h>

int main(int argc, char* argv[])
{
    std::string device;
    uint16_t address;
    boost::program_options::options_description desc("options");
    desc.add_options() //
        ("device,d", boost::program_options::value<std::string>(&device), "I2C file name to which the sensor is connected.") //
        ("address,a", boost::program_options::value<uint16_t>(&address)->default_value(0x77), "BMP280 I2C address(default 0x77).") //
        ("help,h", "produce help message.");
    boost::program_options::variables_map options;
    try {
        boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), options);
        boost::program_options::notify(options);
    } catch (const std::exception& exception) {
        std::cerr << "error: " << exception.what() << std::endl;
        return EXIT_FAILURE;
    }
    if (options.count("help")) {
        std::cout << desc << std::endl;
        return EXIT_SUCCESS;
    }

    // Create sensor.
    std::shared_ptr<BME280> bme = BME280::instance(device, address);
    while (true) {
        const float temperature = bme->getTemperature();
        const float humidity = bme->getHumidity();
        const uint32_t pressureInPPA = bme->getQfePressure();
        const float pressureInHg = BME280::paToHg(pressureInPPA);
        const float dewpoint = BME280::calcDewpoint(humidity, temperature);

        std::cout << "Temperature " << temperature << "°C" << std::endl;
        std::cout << "Humidity " << humidity << "%" << std::endl;
        std::cout << "Pressure " << pressureInHg << "mmHg" << std::endl;
        std::cout << "Dewpoint " << dewpoint << "°C" << std::endl;
        std::cout << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
