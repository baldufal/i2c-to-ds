import smbus
import time

# Define I2C parameters
bus_number = 1
device_address = 0x3f

# Create an instance of the smbus
bus = smbus.SMBus(bus_number)

def write_number(register, value):
    # Write the decimal number to register 0
    bus.write_byte_data(device_address, register, value)

def read_registers():
    # Read all registers from 0 to 20
    registers = [bus.read_byte_data(device_address, i) for i in range(5)]
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


def main():
    try:
        # Get input from the user
        register = int(input("Enter register: "))
        value = int(input("Enter value: "))

        # Write the number to register 0
        write_number(register, value)

        # Wait for 1 second
        time.sleep(1)

        # Read all registers from 0 to 20
        registers = read_registers()

        # Print the registers
        print("Read registers:")
        print_register_values(registers)
        

    except KeyboardInterrupt:
        print("\nTest aborted by user.")
    except Exception as e:
        print(f"An error occurred: {str(e)}")
    finally:
        # Clean up resources
        bus.close()

if __name__ == "__main__":
    main()
