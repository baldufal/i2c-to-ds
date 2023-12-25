import smbus
from crccheck.crc import Crc8Maxim
import time

# Define I2C parameters
bus_number = 1
device_address = 0x3f

# Create an instance of the smbus
bus = smbus.SMBus(bus_number)

def write_number(value):
    # Write the decimal number to register 0
    bus.write_byte_data(device_address, 0, value)

def read_registers():
    # Read all registers from 0 to 20
    registers = [bus.read_byte_data(device_address, i) for i in range(33)]
    return registers
    
def print_register_values(registers):
    # Print the register values in binary, hex, and decimal formats with specified width
    for i, value in enumerate(registers):
        if(i == 5):
            print("")
        binary_str = format(value, '08b')  # 8 bits for binary representation
        hex_str = format(value, '02X')     # 2 characters for hex representation
        if(i>4 and i%2 == 1):
            print(f"Register {i:02d}: Binary: {binary_str}, Hex: {hex_str}, Decimal: {value}   T:{getTemperature(registers, i)}")
        else:
            print(f"Register {i:02d}: Binary: {binary_str}, Hex: {hex_str}, Decimal: {value}")
    print("")
        
def is_length_valid(registers):
    # Check if the value in register 4 specifies the correct number of following registers
    index_4 = 4
    value_4 = registers[index_4]

    # Calculate the expected last non-zero register index
    expected_last_non_zero_index = index_4 + value_4

    # Find the actual last non-zero register index
    last_non_zero_index = max(4, max(i for i, value in enumerate(registers) if value != 0))

    return expected_last_non_zero_index == last_non_zero_index

def compute_crc(data):
    return Crc8Maxim.calc(data)
    
def is_valid_crc8(registers):
    # Calculate CRC8 for the specified range (register 3 plus a number of following registers)
    calculated_crc = compute_crc(bytes(registers[4:4 + registers[4] + 1]))
    return registers[3] == calculated_crc

def getTemperature(registers, index):
    twoBytes = registers[index: index + 2]
    value = int.from_bytes(twoBytes, byteorder='little', signed=True)
    value = value / 16.0
    return value

def main():
    try:
        # Get input from the user
        input_value = int(input("Enter a decimal number to write to register 0: "))

        # Write the number to register 0
        write_number(input_value)

        # Wait for 1 second
        time.sleep(2)

        # Read all registers from 0 to 20
        registers = read_registers()

        # Print the registers
        print("Read registers:")
        print_register_values(registers)
        
        # Check if the value in register 4 specifies the correct number of following registers
        if is_length_valid(registers):
            print("Valid length specified in register 4")
        else:
            print("WARNING: Length in register 4 might be invalid!")
            
        # Check if the CRC8 in register 3 is correct for the specified range
        if is_valid_crc8(registers):
            print("Valid CRC8")
        else:
            print("WARNING: CRC8 does not match!")

    except KeyboardInterrupt:
        print("\nTest aborted by user.")
    except Exception as e:
        print(f"An error occurred: {str(e)}")
    finally:
        # Clean up resources
        bus.close()

if __name__ == "__main__":
    main()
