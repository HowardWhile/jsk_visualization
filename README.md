# jsk_visualization ROS 2 Jazzy Port

This repository is being ported from ROS 1 to ROS 2 Jazzy.

Status legend:

- Done: ported and registered for ROS 2 / Jazzy in this workspace.
- Pending: still needs ROS 2 porting.

## jsk_rviz_plugins

Current ROS 2 plugin description includes:

- Done `jsk_rviz_plugins/CameraInfo`
- Done `jsk_rviz_plugins/OverlayText`
- Done `jsk_rviz_plugins/PieChart`
- Done `jsk_rviz_plugins/String`
- Done `jsk_rviz_plugins/TFTrajectory`

All RViz plugins found in the package:

| Status | Plugin | Kind |
| --- | --- | --- |
| Pending | AmbientSound | Display |
| Pending | BoundingBox | Display |
| Pending | BoundingBoxArray | Display |
| Done | CameraInfo | Display |
| Pending | CancelAction | Panel |
| Pending | CloseAll | Tool |
| Pending | Diagnostics | Display |
| Pending | EmptyServiceCallInterfaceAction | Panel |
| Pending | Footstep | Display |
| Pending | HumanSkeletonArray | Display |
| Pending | LinearGauge | Display |
| Pending | NormalDisplay | Display |
| Pending | ObjectFitOperatorAction | Panel |
| Pending | OpenAll | Tool |
| Pending | OverlayCamera | Display |
| Pending | OverlayDiagnostic | Display |
| Pending | OverlayImage | Display |
| Pending | OverlayMenu | Display |
| Pending | OverlayPicker | Tool |
| Done | OverlayText | Display |
| Pending | PeoplePositionMeasurementArray | Display |
| Pending | Pictogram | Display |
| Pending | PictogramArray | Display |
| Done | PieChart | Display |
| Pending | Plotter2D | Display |
| Pending | PolygonArray | Display |
| Pending | PoseArray | Display |
| Pending | PublishTopic | Panel |
| Pending | QuietInteractiveMarker | Display |
| Pending | RecordAction | Panel |
| Pending | RobotCommandInterfaceAction | Panel |
| Pending | RvizScenePublisher | Display |
| Pending | ScreenshotListener | Tool |
| Pending | SegmentArray | Display |
| Pending | SelectPointCloudPublishAction | Panel |
| Pending | SimpleOccupancyGridArray | Display |
| Done | String | Display |
| Done | TFTrajectory | Display |
| Pending | TabletControllerPanel | Panel |
| Pending | TabletViewController | ViewController |
| Pending | TargetVisualizer | Display |
| Pending | TorusArray | Display |
| Pending | TwistStamped | Display |
| Pending | VideoCapture | Display |
| Pending | YesNoButtonInterface | Panel |

## jsk_rqt_plugins

The package build/install scaffolding has been moved to `ament_cmake`, but the individual RQT plugins still contain ROS 1 Python APIs such as `rospy` / `roslib`.

All RQT plugins found in `jsk_rqt_plugins/plugin.xml`:

| Status | Plugin | Class |
| --- | --- | --- |
| Pending | StatusLight | `jsk_rqt_plugins.status_light.StatusLight` |
| Pending | StringLabel | `jsk_rqt_plugins.label.StringLabel` |
| Pending | ImageView2Plugin | `jsk_rqt_plugins.image_view2_wrapper.ImageView2Plugin` |
| Pending | ServiceButtons | `jsk_rqt_plugins.button.ServiceButtons` |
| Pending | ServiceRadioButtons | `jsk_rqt_plugins.radio_button.ServiceRadioButtons` |
| Pending | DRCEnvironmentViewer | `jsk_rqt_plugins.mini_maxwell.DRCEnvironmentViewer` |
| Pending | Plot3D | `jsk_rqt_plugins.plot.Plot3D` |
| Pending | HistogramPlot | `jsk_rqt_plugins.hist.HistogramPlot` |
| Pending | Plot2D | `jsk_rqt_plugins.plot_2d.Plot2D` |
| Pending | YesNoButton | `jsk_rqt_plugins.yes_no_button.YesNoButton` |
| Pending | ServiceTabbedButtons | `jsk_rqt_plugins.tabbed_button.ServiceTabbedButtons` |
