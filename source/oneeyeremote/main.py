import pygame
import time
import socket


def read_joystick():
    # Initialize pygame
    pygame.init()
    pygame.joystick.init()

    # Check if any joystick/game controller is connected
    if pygame.joystick.get_count() == 0:
        print("No joystick detected")
        return

    # Initialize the first joystick
    joystick = pygame.joystick.Joystick(0)
    joystick.init()

    print(f"Initialized {joystick.get_name()}")
    print(f"Number of axes: {joystick.get_numaxes()}")
    print(f"Number of buttons: {joystick.get_numbuttons()}")

    def map_to_255(value):
        # Map from -1.0...1.0 to 0...255
        return int(((value + 1.0) * 255) / 2.0)

    # Setup TCP connection
    SERVER_IP = "192.168.4.1"  # esp32 oneeye server
    SERVER_PORT = 24

    prevcommand = ""
    s = None
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((SERVER_IP, SERVER_PORT))
        print(f"Connected to {SERVER_IP}:{SERVER_PORT}")

        while True:
            # Handle events
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    return

            # Get axis values and map them to 0-255 range
            axes = [map_to_255(joystick.get_axis(i)) for i in [0, 1, 3, 4]]
            # print("\nAxes:", end=" ")
            #for i, axis in enumerate(axes):
            #    print(f"{i}: {axis:3d}", end=" | ")

            button4 = joystick.get_button(4)
            button5 = joystick.get_button(5)
            # print(" Buttons:", end=" ")
            # print(f"4: {button4} | 5: {button5} ", end=" ")

            command = f"%{axes[0]:02X}{axes[1]:02X}{axes[2]:02X}{axes[3]:02X}{button4:01X}{button5:01X}\n"
            if command != prevcommand:
                prevcommand = command
                # print(command)

                # Send command to TCP server
                try:
                    s.sendall(command.encode('utf-8'))
                except Exception as e:
                    print(f"Error sending data: {e}")
                    # break

            # time.sleep(0.05)  # Small delay to make outpu
            # t readable

    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Connection error: {e}")
    finally:
        if s:
            s.close()
            print("Connection closed.")
        pygame.quit()


if __name__ == "__main__":
    read_joystick()
