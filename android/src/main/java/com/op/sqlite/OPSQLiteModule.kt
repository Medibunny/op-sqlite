package com.op.sqlite

import android.util.Log
import com.facebook.react.bridge.Promise
import com.facebook.react.bridge.ReactApplicationContext
import com.facebook.react.bridge.ReactMethod
import com.facebook.react.bridge.ReactContextBaseJavaModule
import com.facebook.react.bridge.ReadableMap
import java.io.File
import java.io.FileOutputStream
import java.io.InputStream
import java.io.OutputStream
import com.facebook.react.util.RNLog;

//@ReactModule(name = OPSQLiteModule.NAME)
internal class OPSQLiteModule(context: ReactApplicationContext?) : ReactContextBaseJavaModule(context) {
    override fun getName(): String {
        return NAME
    }

    fun getTypedExportedConstants(): MutableMap<String, Any> {
        val constants: MutableMap<String, Any> = HashMap()
        val context = reactApplicationContext
        val dbPath =
                context.getDatabasePath("defaultDatabase")
                        .absolutePath
                        .replace("defaultDatabase", "")
        constants["ANDROID_DATABASE_PATH"] = dbPath
        val filesPath = context.filesDir.absolutePath
        constants["ANDROID_FILES_PATH"] = filesPath
        val externalFilesDir = context.getExternalFilesDir(null)!!.absolutePath
        constants["ANDROID_EXTERNAL_FILES_PATH"] = externalFilesDir
        constants["IOS_DOCUMENT_PATH"] = ""
        constants["IOS_LIBRARY_PATH"] = ""
        return constants
    }

    override fun getConstants(): MutableMap<String, Any>? {
        return getTypedExportedConstants()
    }

    @ReactMethod(isBlockingSynchronousMethod = true)
    fun install(): Boolean {
        return try {
            OPSQLiteBridge.instance.install(reactApplicationContext)
            true
        } catch (exception: Exception) {
            Log.e(NAME, "Install exception: $exception")
            false
        }
    }

    @ReactMethod(isBlockingSynchronousMethod = true)
    fun getDylibPath(bundleId: String, name: String) {
        throw Exception("Do not call getDylibPath on Android")
    }

    @ReactMethod
    fun moveAssetsDatabase(args: ReadableMap, promise: Promise) {
        val filename = args.getString("filename")!!
        val path = args.getString("path") ?: "custom"
        val overwrite = if(args.hasKey("overwrite")) { args.getBoolean("overwrite") } else false
        val context = reactApplicationContext
        val assetsManager = context.assets

        try {
            val databasesFolder =
                context.getDatabasePath("defaultDatabase")
                    .absolutePath
                    .replace("defaultDatabase", "")

            val outputFile = File(databasesFolder, filename)

            var assetStream: InputStream? = null
            try {
                assetStream = assetsManager.open("$path/$filename")
            } catch (e: Exception) {
                // Asset not found
            }

            if (assetStream == null) {
                if (outputFile.exists()) {
                    promise.resolve(true)
                } else {
                    promise.reject("op-sqlite-error", "Asset not found for file: $filename")
                }
                return
            }


            if (outputFile.exists()) {
                if(overwrite) {
                    outputFile.delete()
                } else {
                    promise.resolve(true)
                    return
                }
            }

            val outputStream: OutputStream = FileOutputStream(outputFile)

            // Copy the contents from the input stream to the output stream
            val buffer = ByteArray(1024)
            var length: Int
            while (assetStream.read(buffer).also { length = it } > 0) {
                outputStream.write(buffer, 0, length)
            }

            // Close the streams
            assetStream.close()
            outputStream.close()

            promise.resolve(true)
        } catch (exception: Exception) {
            RNLog.e(this.reactApplicationContext, "Exception: $exception")
            promise.reject("op-sqlite-error", "Failed to move database asset: ${exception.message}", exception)
        }
    }

    override fun invalidate() {
        super.invalidate()
        OPSQLiteBridge.instance.invalidate()
    }

    companion object {
        init {
            System.loadLibrary("op-sqlite")
        }

        const val NAME = "OPSQLite"
    }
}
