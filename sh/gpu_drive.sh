sudo apt update
sudo apt install ubuntu-drivers-common
ubuntu-drivers devices
sudo apt install -y nvidia-driver-470-server
# delete it if you use it in ansible
sudo reboot