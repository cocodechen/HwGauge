sudo rm -rf /usr/local/cuda
sudo rm -rf /usr/local/cuda-12.2

sudo apt-get update
sudo apt-get install build-essential
wget https://developer.download.nvidia.com/compute/cuda/11.4.4/local_installers/cuda_11.4.4_470.82.01_linux.run

sudo sh cuda_11.4.4_470.82.01_linux.run --toolkit --silent --override

echo 'export PATH=/usr/local/cuda-11.4/bin:$PATH' >> ~/.bashrc
echo 'export LD_LIBRARY_PATH=/usr/local/cuda-11.4/lib64:$LD_LIBRARY_PATH' >> ~/.bashrc
# delete it if you use it in ansible
# source ~/.bashrc