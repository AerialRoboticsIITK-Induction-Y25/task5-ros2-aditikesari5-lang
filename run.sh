#!/bin/bash
# Docker Run Script for ROS 2 Jazzy Fleet System

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

IMAGE_NAME="task5-ros2-aditikesari5-lang"
TAG="latest"

echo -e "${YELLOW}========================================${NC}"
echo -e "${YELLOW}Docker Fleet System Runner (Jazzy)${NC}"
echo -e "${YELLOW}========================================${NC}"

if ! command -v docker &> /dev/null; then
    echo -e "${RED}Error: Docker is not installed${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Docker found${NC}"

echo -e "${YELLOW}Building Docker image...${NC}"
docker build -t ${IMAGE_NAME}:${TAG} .

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Image built successfully${NC}"
else
    echo -e "${RED}✗ Image build failed${NC}"
    exit 1
fi

echo ""

echo -e "${YELLOW}Starting container...${NC}"
echo -e "${YELLOW}Press Ctrl+C to stop${NC}"
echo ""

docker run -it \
    --rm \
    --net=host \
    -e ROS_DOMAIN_ID=42 \
    -e DISPLAY=$DISPLAY \
    -v /tmp/.X11-unix:/tmp/.X11-unix \
    ${IMAGE_NAME}:${TAG}

if [ $? -eq 0 ]; then
    echo -e "${GREEN}✓ Container exited successfully${NC}"
else
    echo -e "${RED}✗ Container exited with error${NC}"
fi
