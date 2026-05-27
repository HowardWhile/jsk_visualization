#!/usr/bin/env python3

import sys
from geometry_msgs.msg import Twist, TwistStamped
import rclpy
from rclpy.utilities import remove_ros_args


def main():
    rclpy.init(args=sys.argv)
    node = rclpy.create_node("twist_stamped_add_header")
    args = remove_ros_args(args=sys.argv)[1:]
    if len(args) != 2:
        node.get_logger().error("Usage: twist_stamped_add_header frame_id topic")
        node.destroy_node()
        rclpy.shutdown()
        return

    frame_id, topic = args
    pub = node.create_publisher(TwistStamped, "cmd_vel_stamped", 10)

    def callback(msg):
        output = TwistStamped()
        output.header.stamp = node.get_clock().now().to_msg()
        output.header.frame_id = frame_id
        output.twist = msg
        pub.publish(output)

    sub = node.create_subscription(Twist, topic, callback, 10)
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
