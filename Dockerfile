# ============================================================================
# Stage 1: Builder Stage
# ============================================================================
FROM osrf/ros:jazzy-desktop AS builder

WORKDIR /home/ros_user

RUN apt-get update && apt-get install -y \
    python3-pip \
    git \
    && rm -rf /var/lib/apt/lists/*

# Copy the entire workspace
COPY . /home/ros_user/ros2_ws/src/task5-ros2-aditikesari5-lang

WORKDIR /home/ros_user/ros2_ws

# Create src directory if needed
RUN mkdir -p /home/ros_user/ros2_ws/src/task5-ros2-aditikesari5-lang

# Copy again to ensure it's there
COPY . /home/ros_user/ros2_ws/src/task5-ros2-aditikesari5-lang
#build with jazzy
RUN cd /home/ros_user/ros2_ws && \
    . /opt/ros/jazzy/setup.sh && \



# ============================================================================
# Stage 2: Runtime Stage
# ============================================================================
FROM osrf/ros:jazzy-desktop AS runtime

RUN useradd -m -s /bin/bash ros_user

WORKDIR /home/ros_user/ros2_ws

COPY --from=builder /home/ros_user/ros2_ws/install /home/ros_user/ros2_ws/install

COPY --from=builder /home/ros_user/ros2_ws/src /home/ros_user/ros2_ws/src

RUN chown -R ros_user:ros_user /home/ros_user/ros2_ws

USER ros_user

ENV ROS_DOMAIN_ID=42

# Create entrypoint script with Jazzy paths
RUN mkdir -p /home/ros_user && \
    echo '#!/bin/bash' > /home/ros_user/entrypoint.sh && \
    echo 'set -e' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "=========================================="' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "Entrypoint Script Starting"' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "=========================================="' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "Sourcing ROS 2 jazzy..."' >> /home/ros_user/entrypoint.sh && \
    echo 'source /opt/ros/jazzy/setup.bash' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "Sourcing workspace install..."' >> /home/ros_user/entrypoint.sh && \
    echo 'source /home/ros_user/ros2_ws/install/setup.bash' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "ROS_DISTRO: $ROS_DISTRO"' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "ROS_DOMAIN_ID: $ROS_DOMAIN_ID"' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "=========================================="' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "Starting Fleet System"' >> /home/ros_user/entrypoint.sh && \
    echo 'echo "=========================================="' >> /home/ros_user/entrypoint.sh && \
    echo '' >> /home/ros_user/entrypoint.sh && \
    echo 'exec "$@"' >> /home/ros_user/entrypoint.sh && \
    chmod +x /home/ros_user/entrypoint.sh

ENTRYPOINT ["/home/ros_user/entrypoint.sh"]

CMD ["ros2", "launch", "task5-ros2-aditikesari5-lang", "fleet.launch.py"]
