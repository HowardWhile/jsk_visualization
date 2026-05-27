#!/usr/bin/env python3

import rclpy
from sensor_msgs.msg import CameraInfo


class RelayCameraInfo():
    def __init__(self):
        rclpy.init()
        self.node = rclpy.create_node('relay_camera_info')
        self.node.declare_parameter('frame_id', '')
        self.frame_id = self.node.get_parameter('frame_id').value
        self.pub = self.node.create_publisher(CameraInfo, "output", 10)
        self.sub = self.node.create_subscription(CameraInfo, "input", self.callback, 10)
        try:
            rclpy.spin(self.node)
        finally:
            self.node.destroy_node()
            rclpy.shutdown()

    def callback(self, info):
        info.header.frame_id = self.frame_id
        self.pub.publish(info)


if __name__ == '__main__':
    RelayCameraInfo()
