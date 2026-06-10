from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    """
    Launch file to start all 5 nodes:
    - 3 Drone Nodes (Alpha, Beta, Gamma)
    - 1 Fleet Manager Node
    - 1 Health Monitor Node
    
    Executed with:
    ros2 launch task5-ros2-aditikesari5-lang fleet.launch.py
    """
    
    return LaunchDescription([
        # ====================================================================
        # Alpha Drone Node
        # ====================================================================
        # Starts with 100% battery
        Node(
            package='task5-ros2-aditikesari5-lang',
            executable='drone_node',
            name='alpha_drone_node',
            output='screen',
            parameters=[
                {'drone_name': 'Alpha'},
                {'initial_battery': 100.0},
                {'mission_name': 'Alpha_Delivery'},
            ]
        ),

        # ====================================================================
        # Beta Drone Node
        # ====================================================================
        # Starts with 60% battery (medium)
        Node(
            package='task5-ros2-aditikesari5-lang',
            executable='drone_node',
            name='beta_drone_node',
            output='screen',
            parameters=[
                {'drone_name': 'Beta'},
                {'initial_battery': 60.0},
                {'mission_name': 'Beta_Surveillance'},
            ]
        ),

        # ====================================================================
        # Gamma Drone Node
        # ====================================================================
        # Starts with 35% battery (nearly critical)
        # Will hit critical battery quickly and demonstrate warnings
        Node(
            package='task5-ros2-aditikesari5-lang',
            executable='drone_node',
            name='gamma_drone_node',
            output='screen',
            parameters=[
                {'drone_name': 'Gamma'},
                {'initial_battery': 35.0},
                {'mission_name': 'Gamma_Patrol'},
            ]
        ),

        # ====================================================================
        # Fleet Manager Node
        # ====================================================================
        # Subscribes to all drone topics and prints reports
        Node(
            package='task5-ros2-aditikesari5-lang',
            executable='fleet_manager_node',
            name='fleet_manager_node',
            output='screen',
        ),

        # ====================================================================
        # Health Monitor Node
        # ====================================================================
        # Monitors battery drain rates and health
        Node(
            package='task5-ros2-aditikesari5-lang',
            executable='health_monitor_node',
            name='health_monitor_node',
            output='screen',
        ),
    ])
