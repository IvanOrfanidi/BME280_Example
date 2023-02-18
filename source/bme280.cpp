#include <bme280.h>

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <thread>

#include <err.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <linux/const.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/types.h>

extern int errno;

std::shared_ptr<BME280> BME280::instance(const std::string_view file, uint16_t address, Config config)
{
    std::string device = file.data();
    std::shared_ptr<BME280> i2c;
    if (_interfaces.find(device) == _interfaces.end()) {
        i2c = std::shared_ptr<BME280>(new BME280(device));
        _interfaces[device] = i2c;
    } else {
        i2c = _interfaces[device].lock();
    }

    i2c->_address = address;
    if (!i2c->opened()) {
        i2c->open();
        i2c->check();
        i2c->calibration();
        i2c->setConfig(config);
    }
    return i2c;
}

BME280::BME280(const std::string_view device)
    : _device(device.data())
    , _i2c(CLOSE)
{
}

BME280::~BME280()
{
    close();
    _interfaces.erase(_device);
}

void BME280::close()
{
    if (opened()) {
        if (::close(_i2c) < 0) {
            throw std::runtime_error(std::string("can't close bmp280 device. error is ") + strerror(errno));
        }

        _i2c = CLOSE;
    }
}

void BME280::setConfig(Config config)
{
    _config = config;
    writedByteData(Registers::CONFIG, (_config.standby << 5) | (_config.filter << 2));
    writedByteData(Registers::CTRL_HUM, _config.oversamplingHumidity);
    writedByteData(Registers::CTRL, (_config.oversamplingTemperature << 5) | (_config.oversamplingPressure << 2) | _config.mode);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
}

void BME280::softReset() const
{
    writedByteData(Registers::SOFT_RESET_REG, SOFT_RESET_COMMAND);

    uint16_t timeout = TIMEOUT_FOR_SOFT_RESET;
    while (readByteData(Registers::STATUS) != STATUS_OK) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (--timeout == 0) {
            throw std::runtime_error("failed to soft reset bmp280 sensor");
        }
    }
}

void BME280::calibration()
{
    softReset();

    uint8_t calibrationData[TEMP_PRESS_CALIB_DATA_LEN];
    memset(calibrationData, 0, TEMP_PRESS_CALIB_DATA_LEN);
    readBlockData(TEMP_PRESS_CALIB_DATA_ADDR, calibrationData, TEMP_PRESS_CALIB_DATA_LEN);

    // Temperature
    _calibration.T1 = (uint16_t)((calibrationData[1] << 8) | calibrationData[0]);
    _calibration.T2 = (int16_t)((calibrationData[3] << 8) | calibrationData[2]);
    _calibration.T3 = (int16_t)((calibrationData[5] << 8) | calibrationData[4]);

    // Pressure
    _calibration.P1 = (uint16_t)((calibrationData[7] << 8) | calibrationData[6]);
    _calibration.P2 = (int16_t)((calibrationData[9] << 8) | calibrationData[8]);
    _calibration.P3 = (int16_t)((calibrationData[11] << 8) | calibrationData[10]);
    _calibration.P4 = (int16_t)((calibrationData[13] << 8) | calibrationData[12]);
    _calibration.P5 = (int16_t)((calibrationData[15] << 8) | calibrationData[14]);
    _calibration.P6 = (int16_t)((calibrationData[17] << 8) | calibrationData[16]);
    _calibration.P7 = (int16_t)((calibrationData[19] << 8) | calibrationData[18]);
    _calibration.P8 = (int16_t)((calibrationData[21] << 8) | calibrationData[20]);
    _calibration.P9 = (int16_t)((calibrationData[23] << 8) | calibrationData[22]);

    /* Humidity */
    _calibration.H1 = readByteData(Registers::HUM_CALIB_H1);
    _calibration.H2 = (int16_t)(readByteData(Registers::HUM_CALIB_H2_MSB) << 8 | readByteData(Registers::HUM_CALIB_H2_LSB));
    _calibration.H3 = readByteData(Registers::HUM_CALIB_H3);
    const uint16_t h4 = readByteData(Registers::HUM_CALIB_H4_MSB) << 8 | readByteData(Registers::HUM_CALIB_H4_LSB);
    const uint16_t h5 = readByteData(Registers::HUM_CALIB_H5_MSB) << 8 | readByteData(Registers::HUM_CALIB_H5_LSB);
    _calibration.H4 = (int16_t)((h4 & 0x00FF) << 4 | (h4 & 0x0F00) >> 8);
    _calibration.H5 = (int16_t)(h5 >> 4);
    _calibration.H6 = (int8_t)readByteData(Registers::HUM_CALIB_H6);

    _calibration.updateTime = std::time(nullptr);
}

bool BME280::wasCalibration() const noexcept
{
    return _calibration.updateTime != 0;
}

time_t BME280::getTimeLastCalibration() const noexcept
{
    return _calibration.updateTime;
}

bool BME280::opened() const noexcept
{
    return _i2c > CLOSE;
}

void BME280::open()
{
    _i2c = ::open(_device.data(), O_RDWR);
    if (_i2c < 0) {
        throw std::runtime_error(std::string("can't open bmp280 sensor. error is ") + strerror(errno));
    }

    if (::ioctl(_i2c, I2C_SLAVE, _address) < 0) {
        throw std::runtime_error("tried to set bmp280 sensor address");
    }
}

void BME280::check() const
{
    if (readByteData(Registers::CHIP_ID_REG) != CHIP_ID) {
        throw std::runtime_error("failed to connect bmp280 sensor");
    }
}

int32_t BME280::getRawTemperature() const
{
    const uint8_t msb = readByteData(Registers::TEMP_MSB);
    const uint8_t lsb = readByteData(Registers::TEMP_LSB);
    const uint8_t xlsb = readByteData(Registers::TEMP_XLSB);
    if (msb == 0x80 && lsb == 0x00 && xlsb == 0x00) {
        throw std::runtime_error("failed to read temperature from bmp280 sensor");
    }
    return ((msb << 12) | (lsb << 4) | (xlsb >> 4));
}

int32_t BME280::calcTemperature(int32_t raw) const
{
    const int32_t var1 = ((((raw >> 3) - ((int32_t)_calibration.T1 << 1))) * (int32_t)_calibration.T2) >> 11;
    const int32_t var2 = (((((raw >> 4) - (int32_t)_calibration.T1) * ((raw >> 4) - (int32_t)_calibration.T1)) >> 12) * (int32_t)_calibration.T3) >> 14;
    return var1 + var2;
}

int32_t BME280::getFineTemperature() const
{
    const int32_t raw = getRawTemperature();
    return calcTemperature(raw);
}

// Get real temperature
float BME280::getTemperature() const
{
    const int32_t raw = getRawTemperature();
    const float temp = (calcTemperature(raw) * 5 + 128) >> 8;
    return temp / 100;
}

// Get real humidity
uint32_t BME280::readRawHumidity() const
{
    const uint8_t msb = readByteData(Registers::HUM_MSB);
    const uint8_t lsb = readByteData(Registers::HUM_LSB);
    if (msb == 0x80 && lsb == 0x00) {
        throw std::runtime_error("failed to read humidity from bmp280 sensor");
    }
    return (msb << 8) | lsb;
}

uint32_t BME280::calcHumidity(uint32_t raw) const
{
    int32_t x1 = getFineTemperature() - 76800;
    x1 = ((((raw << 14) - ((int32_t)_calibration.H4 << 20) - ((int32_t)_calibration.H5 * x1)) + 16384) >> 15)
        * (((((((x1 * (int32_t)_calibration.H6) >> 10) * (((x1 * (int32_t)_calibration.H3) >> 11) + 32768)) >> 10) + 2097152) * (int32_t)_calibration.H2 + 8192) >> 14);
    x1 = x1 - (((((x1 >> 15) * (x1 >> 15)) >> 7) * (int32_t)_calibration.H1) >> 4);
    return (x1 >> 12);
}

// Get real humidity
float BME280::getHumidity() const
{
    const uint32_t raw = readRawHumidity();
    const float calc = calcHumidity(raw);
    return calc / 1024;
}

int32_t BME280::getRawPressure() const
{
    const uint8_t msb = readByteData(Registers::PRESS_MSB);
    const uint8_t lsb = readByteData(Registers::PRESS_LSB);
    const uint8_t xlsb = readByteData(Registers::PRESS_XLSB);
    if (msb == 0x80 && lsb == 0x00 && xlsb == 0x00) {
        throw std::runtime_error("failed to read pressure from bmp280 sensor");
    }
    return (msb << 12) | (lsb << 4) | (xlsb >> 4);
}

uint32_t BME280::calcPressure(int32_t raw) const
{
    int64_t var1 = (int64_t)getFineTemperature() - 128000;
    int64_t var2 = var1 * var1 * (int64_t)_calibration.P6;
    var2 = var2 + ((var1 * (int64_t)_calibration.P5) << 17);
    var2 = var2 + (((int64_t)_calibration.P4) << 35);
    var1 = ((var1 * var1 * (int64_t)_calibration.P3) >> 8) + ((var1 * (int64_t)_calibration.P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)_calibration.P1) >> 33;

    int64_t pres = 1048576 - raw;
    pres = (((pres << 31) - var2) * 3125) / var1;
    var1 = ((int64_t)_calibration.P9 * (pres >> 13) * (pres >> 13)) >> 25;
    var2 = ((int64_t)_calibration.P8 * pres) >> 19;

    pres = ((pres + var1 + var2) >> 8) + ((int64_t)_calibration.P7 << 4);
    return pres / 256;
}

// Returns the QNH pressure at sea level at the measuring point in ppa
uint32_t BME280::getQnhPressure(double altitude) const
{
    const double height = pow((1.0 - 2.25577e-5 * altitude), -5.25588);
    const int32_t raw = getRawPressure();
    return calcPressure(raw) * height;
    ;
}

// Get the QFE pressure at the measuring point in ppa
uint32_t BME280::getQfePressure() const
{
    const int32_t raw = getRawPressure();
    return calcPressure(raw);
}

// Fast integer Pa -> mmHg conversion (Pascals to millimeters of mercury)
float BME280::paToHg(float ppa)
{
    return (ppa * 75) / 10000;
}

// Get the dew point in celsius
float BME280::calcDewpoint(float humidity, float temperature)
{
    humidity /= 100;
    const double x = 243.5 * (((17.67 * temperature) / (243.5 + temperature)) + log(humidity));
    const double y = 17.67 - (((17.67 * temperature) / (243.5 + temperature)) + log(humidity));
    return x / y;
}

void BME280::access(uint8_t markers, uint8_t command, uint32_t size, union i2c_smbus_data* data) const
{
    struct i2c_smbus_ioctl_data args {
        markers, command, size, data
    };

    if (::ioctl(_i2c, I2C_SMBUS, &args) < 0) {
        throw std::runtime_error("tried to set device address");
    }
}

void BME280::writedByteData(uint8_t registers, uint8_t value) const
{
    union i2c_smbus_data data;
    data.byte = value;
    access(I2C_SMBUS_WRITE, registers, I2C_SMBUS_BYTE_DATA, &data);
}

uint8_t BME280::readByteData(uint8_t registers) const
{
    union i2c_smbus_data data;
    access(I2C_SMBUS_READ, registers, I2C_SMBUS_BYTE_DATA, &data);
    return data.byte;
}

size_t BME280::readBlockData(uint8_t registers, uint8_t* values, size_t length) const
{
    union i2c_smbus_data data;
    data.block[0] = length;
    access(I2C_SMBUS_READ, registers, I2C_SMBUS_I2C_BLOCK_DATA, &data);

    if (length < data.block[0]) {
        throw std::range_error("data is out of range");
    }

    memcpy(values, &data.block[1], data.block[0]);
    return data.block[0];
}