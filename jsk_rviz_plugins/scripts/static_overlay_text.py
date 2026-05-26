#!/usr/bin/env python3

# it depends on jsk_rviz_plugins

import rclpy
from jsk_rviz_plugins.overlay_text_interface import OverlayTextInterface


if __name__ == "__main__":
    rclpy.init()
    node = rclpy.create_node("static_overlay_text")
    node.declare_parameter("text", "")
    text_interface = OverlayTextInterface(node, "~output")
    text = node.get_parameter("text").value
    node.create_timer(0.1, lambda: text_interface.publish(str(text)))
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()
